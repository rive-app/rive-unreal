// Copyright Rive, Inc. All rights reserved.

#include "RiveEditorModule.h"

#include "AssetToolsModule.h"
#include "RiveFile_AssetTypeActions.h"
#include "RiveTextureThumbnailRenderer.h"
#include "Rive/RiveFile.h"
#include "ThumbnailRendering/ThumbnailManager.h"

#define LOCTEXT_NAMESPACE "RiveEditorModule"

void UE::Rive::Editor::Private::FRiveEditorModuleModule::StartupModule()
{
	UThumbnailManager::Get().RegisterCustomRenderer(URiveFile::StaticClass(), URiveTextureThumbnailRenderer::StaticClass());

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	
	RiveAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Rive")), LOCTEXT("RiveFileCategory", "Rive"));
	RegisterAssetTypeActions(AssetTools, MakeShareable(new FRiveFile_AssetTypeActions(RiveAssetCategory)));
}

void UE::Rive::Editor::Private::FRiveEditorModuleModule::ShutdownModule()
{
	if (UObjectInitialized())
	{
		UThumbnailManager::Get().UnregisterCustomRenderer(URiveFile::StaticClass());
	}
	
	// Unregister all the asset types that we registered
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (int32 Index = 0; Index < CreatedAssetTypeActions.Num(); ++Index)
		{
			AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeActions[Index].ToSharedRef());
		}
	}
	CreatedAssetTypeActions.Empty();
}

void UE::Rive::Editor::Private::FRiveEditorModuleModule::RegisterAssetTypeActions(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
	AssetTools.RegisterAssetTypeActions(Action);
	CreatedAssetTypeActions.Add(Action);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(UE::Rive::Editor::Private::FRiveEditorModuleModule, RiveEditorModule)
