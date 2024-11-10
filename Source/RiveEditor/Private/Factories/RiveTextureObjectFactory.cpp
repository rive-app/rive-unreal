// Copyright Rive, Inc. All rights reserved.

#include "RiveTextureObjectFactory.h"

#include "Editor/EditorEngine.h"
#include "AssetToolsModule.h"
#include "Logs/RiveEditorLog.h"
#include "Rive/RiveTextureObject.h"
#include "UMG/RiveWidget.h"
#include "UObject/SavePackage.h"
#include "HAL/FileManager.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "Rive/RiveFile.h"

extern UNREALED_API class UEditorEngine* GEditor;

#define LOCTEXT_NAMESPACE "FRiveObjectFactory"

FRiveTextureObjectFactory::FRiveTextureObjectFactory(URiveFile* InRiveFile) :
    RiveFile(InRiveFile)
{}

bool FRiveTextureObjectFactory::SaveAsset(URiveTextureObject* InRiveObject)
{
    if (!InRiveObject)
    {
        return false;
    }

    // auto-save asset outside of the editor
    UPackage* const Package = InRiveObject->GetOutermost();

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

    UMetaData* MetaData = Package->GetMetaData();

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

URiveTextureObject* FRiveTextureObjectFactory::CreateRiveTextureObject()
{
    const UPackage* const RivePackage = RiveFile->GetOutermost();
    const FString RivePackageName = RivePackage->GetName();
    const FString FolderName =
        *FPackageName::GetLongPackagePath(RivePackageName);

    const FString BaseWidgetPackageName =
        FolderName + TEXT("/") + RiveFile->GetName();

    FString RiveObjectName;
    FString RiveObjectPackageName;
    const FAssetToolsModule& AssetToolsModule =
        FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>(
            "AssetTools");
    AssetToolsModule.Get().CreateUniqueAssetName(BaseWidgetPackageName,
                                                 FString("_Texture"),
                                                 /*out*/ RiveObjectPackageName,
                                                 /*out*/ RiveObjectName);

    UPackage* RiveObjectPackage = CreatePackage(*RiveObjectPackageName);

    URiveTextureObject* RiveObject =
        NewObject<URiveTextureObject>(RiveObjectPackage,
                                      URiveTextureObject::StaticClass(),
                                      FName(RiveObjectName),
                                      RF_Public | RF_Standalone);
    RiveObject->Initialize(FRiveDescriptor{RiveFile});
    return RiveObject;
}

bool FRiveTextureObjectFactory::Create()
{
    URiveTextureObject* NewObject = CreateRiveTextureObject();

    if (!NewObject)
    {
        return false;
    }

    if (!SaveAsset(NewObject))
    {
        return false;
    }
    return true;
}

namespace
{
// static void SpawnRiveWidgetActor(const FToolMenuContext& MenuContext)
// {
// 	if (const UContentBrowserAssetContextMenuContext* Context =
// UContentBrowserAssetContextMenuContext::FindContextWithAssets(MenuContext))
// 	{
// 		TArray<URiveFile*> RiveFiles =
// Context->LoadSelectedObjects<URiveFile>(); 		for (URiveFile* RiveFile :
// RiveFiles)
// 		{
// 			if (UWorld* World = GEditor->GetEditorWorldContext().World())
// 			{
// 				ARiveWidgetActor* NewActor = Cast<ARiveWidgetActor>(
// 					World->SpawnActor<ARiveWidgetActor>(FVector::ZeroVector,
// FRotator::ZeroRotator, FActorSpawnParameters()));
//
// 				if (NewActor)
// 				{
// 					NewActor->SetWidgetClass(RiveFile->GetWidgetClass());
// 				}
// 			}
// 		}
// 	}
// }
//
// static FDelayedAutoRegisterHelper
// DelayedAutoRegister(EDelayedRegisterRunPhase::EndOfEngineInit,
// []{
// UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
// 	{
// 		FToolMenuOwnerScoped OwnerScoped(UE_MODULE_NAME);
// 		UToolMenu* Menu =
// UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(UTexture2DDynamic::StaticClass());
//
// 		FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
// 		Section.AddDynamicEntry(TEXT("Rive"),
// FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
// 		{
// 			{
// 				const TAttribute<FText> Label =
// LOCTEXT("RiveFile_SpawnRiveWidget", "Spawn Rive Widget Actor");
// const TAttribute<FText> ToolTip = LOCTEXT("RiveFile_SpawnRiveWidgetTooltip",
// "Spawn Rive Widget Actors."); 				const FSlateIcon Icon =
// FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Texture2D"); const
// FToolMenuExecuteAction UIAction =
// FToolMenuExecuteAction::CreateStatic(&SpawnRiveWidgetActor);
//
// 				InSection.AddMenuEntry("RiveFile_SpawnRiveWidget", Label,
// ToolTip, Icon, UIAction);
// 			}
// 		}));
// 	}));
// });
}

#undef LOCTEXT_NAMESPACE
