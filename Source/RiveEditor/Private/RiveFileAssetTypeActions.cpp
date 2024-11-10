// Copyright Rive, Inc. All rights reserved.
#include "RiveFileAssetTypeActions.h"

#include "RiveFileEditor.h"
#include "Rive/RiveFile.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FRiveFileAssetTypeActions::FRiveFileAssetTypeActions(
    EAssetTypeCategories::Type InAssetCategory) :
    AssetCategory(InAssetCategory)
{}

FText FRiveFileAssetTypeActions::GetName() const
{
    return LOCTEXT("FRiveFileAssetTypeActionsName", "Rive File");
}

FColor FRiveFileAssetTypeActions::GetTypeColor() const { return FColor::Red; }

UClass* FRiveFileAssetTypeActions::GetSupportedClass() const
{
    return URiveFile::StaticClass();
}

FText FRiveFileAssetTypeActions::GetAssetDescription(
    const FAssetData& AssetData) const
{
    return FAssetTypeActions_Base::GetAssetDescription(AssetData);
}

bool FRiveFileAssetTypeActions::IsImportedAsset() const { return true; }

void FRiveFileAssetTypeActions::OpenAssetEditor(
    const TArray<UObject*>& InObjects,
    TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
    for (UObject* Obj : InObjects)
    {
        if (URiveFile* RiveFile = Cast<URiveFile>(Obj))
        {
            const TSharedRef<FRiveFileEditor> EditorToolkit =
                MakeShared<FRiveFileEditor>();
            EditorToolkit->Initialize(RiveFile,
                                      EToolkitMode::Standalone,
                                      EditWithinLevelEditor);
        }
    }
}

uint32 FRiveFileAssetTypeActions::GetCategories() { return AssetCategory; }

#undef LOCTEXT_NAMESPACE
