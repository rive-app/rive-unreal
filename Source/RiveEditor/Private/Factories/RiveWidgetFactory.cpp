// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "RiveWidgetFactory.h"

#include "Editor/EditorEngine.h"
#include "AssetToolsModule.h"
#include "ContentBrowserMenuContexts.h"
#include "PackageTools.h"
#include "WidgetBlueprint.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Game/RiveWidgetActor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Logs/RiveEditorLog.h"
#include "Templates/WidgetTemplateClass.h"
#include "UMG/RiveWidget.h"
#include "UObject/SavePackage.h"
#include "HAL/FileManager.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "Components/CanvasPanel.h"
#include "Rive/RiveFile.h"

extern UNREALED_API class UEditorEngine* GEditor;

#define LOCTEXT_NAMESPACE "FRiveWidgetFactory"

FRiveWidgetFactory::FRiveWidgetFactory(URiveFile* InRiveFile) :
    RiveFile(InRiveFile)
{}

bool FRiveWidgetFactory::SaveAsset(UWidgetBlueprint* InWidgetBlueprint)
{
    if (!InWidgetBlueprint)
    {
        return false;
    }

    // auto-save asset outside of the editor
    UPackage* const Package = InWidgetBlueprint->GetOutermost();

    FString const PackageName = Package->GetName();

    FString const PackageFileName = FPackageName::LongPackageNameToFilename(
        PackageName,
        FPackageName::GetAssetPackageExtension());

    if (IFileManager::Get().IsReadOnly(*PackageFileName))
    {
        UE_LOG(LogRiveEditor,
               Error,
               TEXT("Could not save read only file: %s"),
               *PackageFileName);

        return false;
    }

    double StartTime = FPlatformTime::Seconds();

    FSavePackageArgs SaveArgs;

    SaveArgs.TopLevelFlags = RF_Standalone;

    SaveArgs.SaveFlags = SAVE_NoError | SAVE_Async;

    UPackage::SavePackage(Package, NULL, *PackageFileName, SaveArgs);

    const double ElapsedTime = FPlatformTime::Seconds() - StartTime;

    UE_LOG(LogRiveEditor,
           Log,
           TEXT("Saved %s in %0.2f seconds"),
           *PackageName,
           ElapsedTime);

    Package->MarkPackageDirty();

    return true;
}

bool FRiveWidgetFactory::CreateWidgetStructure(
    UWidgetBlueprint* InWidgetBlueprint)
{
    if (!IsValid(InWidgetBlueprint))
    {
        UE_LOG(
            LogRiveEditor,
            Error,
            TEXT(
                " FRiveWidgetFactory::CreateWidgetStructure Invalid Widget Blueprint"));
        return false;
    }

    // Create the desired root widget specified by the project
    if (InWidgetBlueprint->WidgetTree->RootWidget == nullptr)
    {
        UWidget* Root = InWidgetBlueprint->WidgetTree->ConstructWidget<UWidget>(
            UCanvasPanel::StaticClass());

        InWidgetBlueprint->WidgetTree->RootWidget = Root;

        UWidget* Widget = FWidgetTemplateClass(URiveWidget::StaticClass())
                              .Create(InWidgetBlueprint->WidgetTree);

        if (URiveWidget* RiveWidget = Cast<URiveWidget>(Widget))
        {
            FString StateMachineName = "";
            FString ArtboardName = "";

            if (RiveFile->ArtboardDefinitions.Num())
            {
                ArtboardName = RiveFile->ArtboardDefinitions[0].Name;
                if (RiveFile->ArtboardDefinitions[0].StateMachineNames.Num())
                    StateMachineName =
                        RiveFile->ArtboardDefinitions[0].StateMachineNames[0];
            }

            RiveWidget->RiveDescriptor =
                FRiveDescriptor{RiveFile,
                                ArtboardName,
                                StateMachineName,
                                false,
                                ERiveFitType::Contain,
                                ERiveAlignment::Center};
        }

        if (UCanvasPanel* RootWidget = Cast<UCanvasPanel>(Root))
        {
            UCanvasPanelSlot* NewSlot = RootWidget->AddChildToCanvas(Widget);

            check(NewSlot);

            NewSlot->SetAnchors(FAnchors(0, 0, 1, 1));

            NewSlot->SetOffsets(FMargin(0, 0, 0, 0));
        }

        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(
            InWidgetBlueprint);

        return true;
    }

    return false;
}

UWidgetBlueprint* FRiveWidgetFactory::CreateWidgetBlueprint()
{
    UClass* CurrentParentClass = UUserWidget::StaticClass();

    if ((CurrentParentClass == nullptr) ||
        !FKismetEditorUtilities::CanCreateBlueprintOfClass(
            CurrentParentClass) ||
        !CurrentParentClass->IsChildOf(UUserWidget::StaticClass()))
    {
        FFormatNamedArguments Args;

        Args.Add(TEXT("ClassName"),
                 CurrentParentClass
                     ? FText::FromString(CurrentParentClass->GetName())
                     : LOCTEXT("Null", "(null)"));

        FMessageDialog::Open(
            EAppMsgType::Ok,
            FText::Format(LOCTEXT("CannotCreateWidgetBlueprint",
                                  "Cannot create a Widget Blueprint based on "
                                  "the class '{ClassName}'."),
                          Args));
        return nullptr;
    }

    const UPackage* const RivePackage = RiveFile->GetOutermost();
    const FString RivePackageName = RivePackage->GetName();
    const FString RiveWidgetFolderName =
        *FPackageName::GetLongPackagePath(RivePackageName);

    const FString BaseWidgetPackageName =
        RiveWidgetFolderName + TEXT("/") + RiveFile->GetName();

    FString RiveWidgetName;
    FString RiveWidgetPackageName;
    const FAssetToolsModule& AssetToolsModule =
        FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>(
            "AssetTools");
    AssetToolsModule.Get().CreateUniqueAssetName(BaseWidgetPackageName,
                                                 FString("_Widget"),
                                                 /*out*/ RiveWidgetPackageName,
                                                 /*out*/ RiveWidgetName);

    UPackage* RiveWidgetPackage = CreatePackage(*RiveWidgetPackageName);

    return CastChecked<UWidgetBlueprint>(
        FKismetEditorUtilities::CreateBlueprint(
            CurrentParentClass,
            RiveWidgetPackage,
            FName(RiveWidgetName),
            BPTYPE_Normal,
            UWidgetBlueprint::StaticClass(),
            UWidgetBlueprintGeneratedClass::StaticClass()));
}

bool FRiveWidgetFactory::Create()
{
    UWidgetBlueprint* NewBP = CreateWidgetBlueprint();

    if (!NewBP)
    {
        return false;
    }

    if (!SaveAsset(NewBP))
    {
        return false;
    }

    if (!CreateWidgetStructure(NewBP))
    {
        return false;
    }

    // Compile BP
    FCompilerResultsLog LogResults;

    constexpr EBlueprintCompileOptions CompileOptions =
        EBlueprintCompileOptions::None;

    FKismetEditorUtilities::CompileBlueprint(NewBP,
                                             CompileOptions,
                                             &LogResults);

    return true;
}

#undef LOCTEXT_NAMESPACE
