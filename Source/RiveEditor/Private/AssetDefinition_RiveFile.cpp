// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "AssetDefinition_RiveFile.h"

#include "ContentBrowserMenuContexts.h"
#include "EditorAssetLibrary.h"
// #include "RiveFileAssetToolkit.h"
#include "FileHelpers.h"
#include "ObjectTools.h"
#include "PackageTools.h"
#include "RiveFileEditor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/RiveWidgetFactory.h"
#include "Logs/RiveEditorLog.h"
#include "Rive/RiveFile.h"

#include "EditorFramework/AssetImportData.h"
#include "Factories/RiveRenderTargetFactory.h"
#include "Rive/RiveUtils.h"

#define LOCTEXT_NAMESPACE "URiveFile_AssetDefinitionDefault"

namespace MenuExtension_RiveFile
{
void ExecuteCreateWidget(const FToolMenuContext& InContext)
{
    const UContentBrowserAssetContextMenuContext* CBContext =
        UContentBrowserAssetContextMenuContext::FindContextWithAssets(
            InContext);

    TArray<URiveFile*> RiveFiles = CBContext->LoadSelectedObjects<URiveFile>();
    for (URiveFile* RiveFile : RiveFiles)
    {
        if (IsValid(RiveFile))
        {
            FRiveWidgetFactory(RiveFile).Create();
        }
    }
}

void ExecuteCreateRenderTarget(const FToolMenuContext& InContext)
{
    const UContentBrowserAssetContextMenuContext* CBContext =
        UContentBrowserAssetContextMenuContext::FindContextWithAssets(
            InContext);

    TArray<URiveFile*> RiveFileObjects =
        CBContext->LoadSelectedObjects<URiveFile>();
    for (URiveFile* RiveFile : RiveFileObjects)
    {
        if (IsValid(RiveFile))
        {
            FRiveRenderTargetFactory(RiveFile).Create();
        }
    }
}

void CleanGeneratedFilesAndClasses(URiveFile* File)
{
    if (!IsValid(File))
    {
        UE_LOG(LogRiveEditor,
               Error,
               TEXT("Unable to clean generated files and classes for the "
                    "RiveFile: it is invalid"));
        return;
    }

    const FScopedTransaction Transaction(
        NSLOCTEXT("RiveFileAssetActions",
                  "CleanFileGeneratedObjects",
                  "Clean File Generated Objects"));

    TArray<FString> ViewModelNames;
    // This is a map that allows for replacing references and deleting all
    // objects of a type with the key being the "correct" asset to use in place
    // of the deleted ones
    using AssetReference = TPair<UObject*, TArray<UObject*>>;
    TMap<FName, AssetReference> AssetMap;
    for (auto& Definition : File->ViewModelDefinitions)
    {
        ViewModelNames.Add(Definition.Name);
        AssetReference Reference;
        Reference.Key = File->GetGeneratedClassForViewModel(*Definition.Name);
        Reference.Value = {};
        AssetMap.Add(*Definition.Name, Reference);
    }

    for (TObjectIterator<UBlueprint> It; It; ++It)
    {
        UBlueprint* GeneratedBlueprint = *It;
        const FString ObjectName = GeneratedBlueprint->GetName();

        if (ViewModelNames.Contains(ObjectName))
        {
            auto ReferenceEntry = AssetMap.Find(*ObjectName);
            if (!ensure(ReferenceEntry))
            {
                UE_LOG(LogRiveEditor,
                       Error,
                       TEXT("No asset map entry for: %s"),
                       *ObjectName);
                continue;
            }

            // This is the good class, so we don't want to delete it.
            if (ReferenceEntry->Key == GeneratedBlueprint->GeneratedClass)
                continue;

            ReferenceEntry->Value.Add(*It);
        }
    }

    for (auto& AssetEntry : AssetMap)
    {
        if (AssetEntry.Value.Value.IsEmpty())
            continue;
        UE_LOG(LogRiveEditor,
               Verbose,
               TEXT("Replacing references of view model blueprints of type %s"),
               *AssetEntry.Key.ToString())
        ObjectTools::ConsolidateObjects(AssetEntry.Value.Key,
                                        AssetEntry.Value.Value);
    }

    // 3. Ensure everything was Saved to disk.
    const bool bPromptUserToSave = false;
    const bool bSaveMapPackages = true;
    const bool bSaveContentPackages = true;
    const bool bFastSave = false;
    const bool bNotifyNoPackagesSaved = false;
    const bool bCanBeDeclined = false;
    FEditorFileUtils::SaveDirtyPackages(bPromptUserToSave,
                                        bSaveMapPackages,
                                        bSaveContentPackages,
                                        bFastSave,
                                        bNotifyNoPackagesSaved,
                                        bCanBeDeclined);
}

void ExecuteCleanFileGeneratedObjects(const FToolMenuContext& InContext)
{
    const UContentBrowserAssetContextMenuContext* CBContext =
        UContentBrowserAssetContextMenuContext::FindContextWithAssets(
            InContext);

    TArray<URiveFile*> RiveFileObjects =
        CBContext->LoadSelectedObjects<URiveFile>();
    for (URiveFile* RiveFile : RiveFileObjects)
    {
        if (IsValid(RiveFile))
        {
            if (!RiveFile->GetHasData())
            {
                UE_LOG(LogRiveEditor,
                       Error,
                       TEXT("Unable to clean generated files and classes for "
                            "the RiveFile: it has no data"));
                continue;
            }
            CleanGeneratedFilesAndClasses(RiveFile);
        }
    }
}

static FDelayedAutoRegisterHelper DelayedAutoRegister(
    EDelayedRegisterRunPhase::EndOfEngineInit,
    [] {
        UToolMenus::RegisterStartupCallback(
            FSimpleMulticastDelegate::FDelegate::CreateLambda([]() {
                FToolMenuOwnerScoped OwnerScoped(UE_MODULE_NAME);
                UToolMenu* Menu =
                    UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(
                        URiveFile::StaticClass());

                FToolMenuSection& Section =
                    Menu->FindOrAddSection("GetAssetActions");
                Section.AddDynamicEntry(
                    NAME_None,
                    FNewToolMenuSectionDelegate::CreateLambda(
                        [](FToolMenuSection& InSection) {
                            if (const UContentBrowserAssetContextMenuContext*
                                    Context =
                                        UContentBrowserAssetContextMenuContext::
                                            FindContextWithAssets(InSection))
                            {
                                {
                                    const TAttribute<FText> Label =
                                        LOCTEXT("RiveFile_CreateWidget",
                                                "Create Rive Widget");
                                    const TAttribute<FText> ToolTip =
                                        LOCTEXT("RiveFile_CreateWidgetTooltip",
                                                "Creates a new Rive Widget "
                                                "asset using this file.");
                                    const FSlateIcon Icon = FSlateIcon(
                                        FAppStyle::GetAppStyleSetName(),
                                        "ClassIcon.WidgetBlueprint");
                                    const FToolMenuExecuteAction UIAction =
                                        FToolMenuExecuteAction::CreateStatic(
                                            &ExecuteCreateWidget);
                                    InSection.AddMenuEntry(
                                        "RiveFile_CreateWidget",
                                        Label,
                                        ToolTip,
                                        Icon,
                                        UIAction);
                                }

                                {
                                    const TAttribute<FText> Label =
                                        LOCTEXT("RiveFile_CreateRenderTarget",
                                                "Create Rive Render Target");
                                    const TAttribute<FText> ToolTip =
                                        LOCTEXT("RiveFile_CreateWidgetTooltip",
                                                "Creates a new Rive Render "
                                                "Target using this file.");
                                    const FSlateIcon Icon = FSlateIcon(
                                        FAppStyle::GetAppStyleSetName(),
                                        "ClassIcon.Texture2D");
                                    const FToolMenuExecuteAction UIAction =
                                        FToolMenuExecuteAction::CreateStatic(
                                            &ExecuteCreateRenderTarget);
                                    InSection.AddMenuEntry(
                                        "RiveFile_CreateRenderTarget",
                                        Label,
                                        ToolTip,
                                        Icon,
                                        UIAction);
                                }

                                {
                                    const TAttribute<FText> Label = LOCTEXT(
                                        "RiveFile_CleanGeneratedClasses",
                                        "Clean Generated Classes");
                                    const TAttribute<FText> ToolTip = LOCTEXT(
                                        "RiveFile_CleanGeneratedClassesTooltip",
                                        "Deletes all generated objects that no "
                                        "longer have a file association.");
                                    const FSlateIcon Icon = FSlateIcon(
                                        FAppStyle::GetAppStyleSetName(),
                                        "ClassIcon.Default");
                                    const FToolMenuExecuteAction UIAction =
                                        FToolMenuExecuteAction::CreateStatic(
                                            &ExecuteCleanFileGeneratedObjects);
                                    InSection.AddMenuEntry(
                                        "RiveFile_CleanGeneratedClasses",
                                        Label,
                                        ToolTip,
                                        Icon,
                                        UIAction);
                                }
                            }
                        }));
            }));
    });
} // namespace MenuExtension_RiveFile

FText UAssetDefinition_RiveFile::GetAssetDisplayName() const
{
    return LOCTEXT("AssetTypeActions_RiveFile", "Rive File");
}

FLinearColor UAssetDefinition_RiveFile::GetAssetColor() const
{
    return FLinearColor::Red;
}

TSoftClassPtr<> UAssetDefinition_RiveFile::GetAssetClass() const
{
    return URiveFile::StaticClass();
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_RiveFile::
    GetAssetCategories() const
{
    static const FAssetCategoryPath Categories[] = {EAssetCategoryPaths::Misc};

    return Categories;
}

EAssetCommandResult UAssetDefinition_RiveFile::OpenAssets(
    const FAssetOpenArgs& OpenArgs) const
{
    for (URiveFile* RiveFile : OpenArgs.LoadObjects<URiveFile>())
    {
        const TSharedRef<FRiveFileEditor> EditorToolkit =
            MakeShared<FRiveFileEditor>();
        EditorToolkit->Initialize(RiveFile,
                                  OpenArgs.GetToolkitMode(),
                                  OpenArgs.ToolkitHost);
    }

    return EAssetCommandResult::Handled;
}

EAssetCommandResult UAssetDefinition_RiveFile::GetSourceFiles(
    const FAssetSourceFilesArgs& InArgs,
    TFunctionRef<bool(const FAssetSourceFilesResult& InSourceFile)>
        SourceFileFunc) const
{
    for (URiveFile* RiveFile : InArgs.LoadObjects<URiveFile>())
    {
        FAssetSourceFilesResult ImportInfoResult;
        ImportInfoResult.FilePath =
            RiveFile->AssetImportData->GetFirstFilename();
        if (SourceFileFunc(ImportInfoResult))
        {
            return EAssetCommandResult::Handled;
        }
    }
    return EAssetCommandResult::Unhandled;
}

#undef LOCTEXT_NAMESPACE
