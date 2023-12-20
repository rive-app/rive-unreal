// Copyright Rive, Inc. All rights reserved.

#include "RiveFile_AssetDefinitionDefault.h"

#include "RiveAssetEditor.h"
#include "Rive/RiveFile.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FText URiveFile_AssetDefinitionDefault::GetAssetDisplayName() const
{
	return LOCTEXT("AssetTypeActions_SmartObjectDefinition", "SmartObject Definition");
}

FLinearColor URiveFile_AssetDefinitionDefault::GetAssetColor() const
{
	return FLinearColor::Red;
}

TSoftClassPtr<> URiveFile_AssetDefinitionDefault::GetAssetClass() const
{
	return URiveFile::StaticClass();
}

TConstArrayView<FAssetCategoryPath> URiveFile_AssetDefinitionDefault::GetAssetCategories() const
{
	static const FAssetCategoryPath Categories[] = { EAssetCategoryPaths::Misc };

	return Categories;
}

EAssetCommandResult URiveFile_AssetDefinitionDefault::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();

	for (URiveFile* Definition : OpenArgs.LoadObjects<URiveFile>())
	{
		URiveAssetEditor* AssetEditor = NewObject<URiveAssetEditor>(AssetEditorSubsystem, NAME_None, RF_Transient);
		
		AssetEditor->SetObjectToEdit(Definition);
		
		AssetEditor->Initialize();
	}

	return EAssetCommandResult::Handled;
}

#undef LOCTEXT_NAMESPACE
