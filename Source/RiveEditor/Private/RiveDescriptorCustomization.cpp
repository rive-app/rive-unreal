// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "RiveDescriptorCustomization.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "Rive/RiveDescriptor.h"
#include "Rive/RiveFile.h"

TSharedRef<IPropertyTypeCustomization> FRiveDescriptorCustomization::
    MakeInstance()
{
    return MakeShared<FRiveDescriptorCustomization>();
}

void FRiveDescriptorCustomization::CustomizeHeader(
    TSharedRef<IPropertyHandle> StructPropertyHandle,
    FDetailWidgetRow& HeaderRow,
    IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    HeaderRow.NameContent()[StructPropertyHandle->CreatePropertyNameWidget()];
}

void FRiveDescriptorCustomization::CustomizeChildren(
    TSharedRef<IPropertyHandle> StructPropertyHandle,
    IDetailChildrenBuilder& ChildBuilder,
    IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    RiveFileHandle = StructPropertyHandle->GetChildHandle(
        GET_MEMBER_NAME_CHECKED(FRiveDescriptor, RiveFile));
    ArtboardNameHandle = StructPropertyHandle->GetChildHandle(
        GET_MEMBER_NAME_CHECKED(FRiveDescriptor, ArtboardName));

    uint32 NumChildren = 0;
    StructPropertyHandle->GetNumChildren(NumChildren);
    for (uint32 Index = 0; Index < NumChildren; ++Index)
    {
        TSharedRef<IPropertyHandle> ChildHandle =
            StructPropertyHandle->GetChildHandle(Index).ToSharedRef();
        const FName ChildName = ChildHandle->GetProperty()->GetFName();

        if (ChildName == GET_MEMBER_NAME_CHECKED(FRiveDescriptor, ArtboardName))
        {
            ChildBuilder.AddCustomRow(ChildHandle->GetPropertyDisplayName())
                .NameContent()[ChildHandle->CreatePropertyNameWidget()]
                .ValueContent()[PropertyCustomizationHelpers::
                                    MakePropertyComboBox(FPropertyComboBoxArgs(
                                        ChildHandle,
                                        FOnGetPropertyComboBoxStrings::CreateSP(
                                            this,
                                            &FRiveDescriptorCustomization::
                                                OnGetArtboardNames)))];
        }
        else if (ChildName ==
                 GET_MEMBER_NAME_CHECKED(FRiveDescriptor, StateMachineName))
        {
            ChildBuilder.AddCustomRow(ChildHandle->GetPropertyDisplayName())
                .NameContent()[ChildHandle->CreatePropertyNameWidget()]
                .ValueContent()[PropertyCustomizationHelpers::
                                    MakePropertyComboBox(FPropertyComboBoxArgs(
                                        ChildHandle,
                                        FOnGetPropertyComboBoxStrings::CreateSP(
                                            this,
                                            &FRiveDescriptorCustomization::
                                                OnGetStateMachineNames)))];
        }
        else
        {
            ChildBuilder.AddProperty(ChildHandle);
        }
    }
}

URiveFile* FRiveDescriptorCustomization::GetRiveFile() const
{
    UObject* Object = nullptr;
    if (RiveFileHandle.IsValid() &&
        RiveFileHandle->GetValue(Object) == FPropertyAccess::Success)
    {
        return Cast<URiveFile>(Object);
    }

    return nullptr;
}

void FRiveDescriptorCustomization::OnGetArtboardNames(
    TArray<TSharedPtr<FString>>& OutStrings,
    TArray<TSharedPtr<SToolTip>>& OutToolTips,
    TArray<bool>& OutRestrictedItems) const
{
    if (URiveFile* RiveFile = GetRiveFile())
    {
        for (const FArtboardDefinition& Artboard :
             RiveFile->ArtboardDefinitions)
        {
            OutStrings.Add(MakeShared<FString>(Artboard.Name));
            OutRestrictedItems.Add(false);
        }
    }
}

void FRiveDescriptorCustomization::OnGetStateMachineNames(
    TArray<TSharedPtr<FString>>& OutStrings,
    TArray<TSharedPtr<SToolTip>>& OutToolTips,
    TArray<bool>& OutRestrictedItems) const
{
    URiveFile* RiveFile = GetRiveFile();
    if (RiveFile == nullptr || !ArtboardNameHandle.IsValid())
    {
        return;
    }

    FString ArtboardName;
    ArtboardNameHandle->GetValue(ArtboardName);

    for (const FArtboardDefinition& Artboard : RiveFile->ArtboardDefinitions)
    {
        if (Artboard.Name.Equals(ArtboardName))
        {
            for (const FString& StateMachineName : Artboard.StateMachineNames)
            {
                OutStrings.Add(MakeShared<FString>(StateMachineName));
                OutRestrictedItems.Add(false);
            }
            break;
        }
    }
}
