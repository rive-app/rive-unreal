// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class URiveFile;
class SToolTip;

class RIVEEDITOR_API FRiveDescriptorCustomization
    : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    virtual void CustomizeHeader(
        TSharedRef<IPropertyHandle> StructPropertyHandle,
        FDetailWidgetRow& HeaderRow,
        IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    virtual void CustomizeChildren(
        TSharedRef<IPropertyHandle> StructPropertyHandle,
        IDetailChildrenBuilder& ChildBuilder,
        IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
    URiveFile* GetRiveFile() const;
    void OnGetArtboardNames(TArray<TSharedPtr<FString>>& OutStrings,
                            TArray<TSharedPtr<SToolTip>>& OutToolTips,
                            TArray<bool>& OutRestrictedItems) const;
    void OnGetStateMachineNames(TArray<TSharedPtr<FString>>& OutStrings,
                                TArray<TSharedPtr<SToolTip>>& OutToolTips,
                                TArray<bool>& OutRestrictedItems) const;

    TSharedPtr<IPropertyHandle> RiveFileHandle;
    TSharedPtr<IPropertyHandle> ArtboardNameHandle;
};
