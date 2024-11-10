// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "AssetTypeActions_Base.h"

/**
 *
 */
class RIVEEDITOR_API FRiveFileAssetTypeActions : public FAssetTypeActions_Base
{
public:
    FRiveFileAssetTypeActions(EAssetTypeCategories::Type InAssetCategory);

    // IAssetTypeActions interface
    virtual FText GetName() const override;
    virtual FColor GetTypeColor() const override;
    virtual UClass* GetSupportedClass() const override;
    virtual FText GetAssetDescription(
        const FAssetData& AssetData) const override;
    virtual bool IsImportedAsset() const override;
    virtual void OpenAssetEditor(
        const TArray<UObject*>& InObjects,
        TSharedPtr<IToolkitHost> EditWithinLevelEditor) override;
    virtual uint32 GetCategories() override;
    // End of IAssetTypeActions interface

private:
    EAssetTypeCategories::Type AssetCategory;
};
