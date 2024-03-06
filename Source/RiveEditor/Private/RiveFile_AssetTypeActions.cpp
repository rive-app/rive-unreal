// Copyright Rive, Inc. All rights reserved.


#include "RiveFile_AssetTypeActions.h"

#include "Interfaces/ITextureEditorModule.h"
#include "Interfaces/ITextureEditorModule.h"
#include "Rive/RiveFile.h"

#define LOCTEXT_NAMESPACE "RiveEditorModule"

FRiveFile_AssetTypeActions::FRiveFile_AssetTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: AssetCategory(InAssetCategory)
{
}

FText FRiveFile_AssetTypeActions::GetName() const
{
	return LOCTEXT("FRiveFile_AssetTypeActions_Name", "Rive File");
}

FColor FRiveFile_AssetTypeActions::GetTypeColor() const
{
	return FColor::Red;
}

UClass* FRiveFile_AssetTypeActions::GetSupportedClass() const
{
	return URiveFile::StaticClass();
}

FText FRiveFile_AssetTypeActions::GetAssetDescription(const FAssetData& AssetData) const
{
	return FAssetTypeActions_Base::GetAssetDescription(AssetData);
}

bool FRiveFile_AssetTypeActions::IsImportedAsset() const
{
	return true;
}

void FRiveFile_AssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	if (ensure(InObjects.Num() == 1) && InObjects[0]->IsA<URiveFile>())
	{
		ITextureEditorModule* TextureEditorModule = &FModuleManager::LoadModuleChecked<ITextureEditorModule>("TextureEditor");
		TextureEditorModule->CreateTextureEditor(EToolkitMode::Standalone, EditWithinLevelEditor, Cast<URiveFile>(InObjects[0]));
	}
	else
	{
		FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);
	}
}

uint32 FRiveFile_AssetTypeActions::GetCategories()
{
	return AssetCategory;
}
