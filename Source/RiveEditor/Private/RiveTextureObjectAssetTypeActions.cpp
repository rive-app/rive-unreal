// Copyright Rive, Inc. All rights reserved.

#include "RiveTextureObjectAssetTypeActions.h"

#include "RiveTextureObjectEditor.h"
#include "Rive/RiveTextureObject.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FRiveTextureObjectAssetTypeActions::FRiveTextureObjectAssetTypeActions(
    EAssetTypeCategories::Type InAssetCategory) :
    AssetCategory(InAssetCategory)
{}

FText FRiveTextureObjectAssetTypeActions::GetName() const
{
    return LOCTEXT("FRiveTextureObjectAssetTypeActions", "Rive Texture Object");
}

FColor FRiveTextureObjectAssetTypeActions::GetTypeColor() const
{
    return FColor::Red;
}

UClass* FRiveTextureObjectAssetTypeActions::GetSupportedClass() const
{
    return URiveTextureObject::StaticClass();
}

FText FRiveTextureObjectAssetTypeActions::GetAssetDescription(
    const FAssetData& AssetData) const
{
    return FAssetTypeActions_Base::GetAssetDescription(AssetData);
}

bool FRiveTextureObjectAssetTypeActions::IsImportedAsset() const
{
    return true;
}

void FRiveTextureObjectAssetTypeActions::OpenAssetEditor(
    const TArray<UObject*>& InObjects,
    TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
    for (UObject* Obj : InObjects)
    {
        if (URiveTextureObject* RiveTextureObject =
                Cast<URiveTextureObject>(Obj))
        {
            const TSharedRef<FRiveTextureObjectEditor> EditorToolkit =
                MakeShared<FRiveTextureObjectEditor>();
            EditorToolkit->Initialize(RiveTextureObject,
                                      EToolkitMode::Standalone,
                                      EditWithinLevelEditor);
        }
    }
}

uint32 FRiveTextureObjectAssetTypeActions::GetCategories()
{
    return AssetCategory;
}

#undef LOCTEXT_NAMESPACE