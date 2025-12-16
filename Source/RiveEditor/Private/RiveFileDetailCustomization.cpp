// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "RiveFileDetailCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Logs/RiveEditorLog.h"
#include "Rive/RiveArtboard.h"
#include "Rive/RiveFile.h"
#include "Rive/RiveViewModel.h"
#include "Styling/SlateStyleMacros.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include <string>

#include "EditorFontGlyphs.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "PropertyCustomizationHelpers.h"

THIRD_PARTY_INCLUDES_START
#include "rive/animation/state_machine_input.hpp"
#include "rive/animation/state_machine_input_instance.hpp"
#include "rive/generated/animation/state_machine_bool_base.hpp"
#include "rive/generated/animation/state_machine_number_base.hpp"
#include "rive/generated/animation/state_machine_trigger_base.hpp"
THIRD_PARTY_INCLUDES_END

#define LOCTEXT_NAMESPACE "RiveArtboardDetailCustomization"

namespace RiveArtboardDetailCustomizationPrivate
{
static bool IsTrigger(rive::SMIInput* Input)
{
    return Input->input()->is<rive::StateMachineTriggerBase>();
}

static bool IsNumber(rive::SMIInput* Input)
{
    return Input->input()->is<rive::StateMachineNumberBase>();
}

static bool IsBool(rive::SMIInput* Input)
{
    return Input->input()->is<rive::StateMachineBoolBase>();
}

static FLinearColor GetColorForInput(rive::SMIInput* Input)
{
    // colors retrieved from PropertyHelpers
    if (IsBool(Input))
    {
        return FLinearColor(0.300000f, 0.0f, 0.0f, 1.0f);
    }

    if (IsNumber(Input))
    {
        return FLinearColor(0.357667f, 1.0f, 0.060000f, 1.0f);
    }

    if (IsTrigger(Input))
    {
        return FLinearColor(0.0f, 0.349f, 0.79f, 1.0f);
    }

    return FLinearColor::White;
}

static FString GetTypeStringForInput(rive::SMIInput* Input)
{
    if (IsBool(Input))
    {
        return FString("Bool");
    }

    if (IsNumber(Input))
    {
        return FString("Number");
    }

    if (IsTrigger(Input))
    {
        return FString("Trigger");
    }

    return FString("Unknown");
}

static FString GetIconForPropertyType(ERiveDataType Type)
{
    switch (Type)
    {
        case ERiveDataType::Artboard:
            return FString(TEXT("\xf247"));
        case ERiveDataType::AssetImage:
            return FString(TEXT("\xf03e"));
        case ERiveDataType::Boolean:
            return FString(TEXT("\xf046")); // FontAwesome icon for check-square
        case ERiveDataType::Color:
            return FString(TEXT("\xf53f")); // FontAwesome icon for palette
        case ERiveDataType::EnumType:
            return FString(TEXT("\xf022")); // FontAwesome icon for list
        case ERiveDataType::Number:
            return FString(TEXT("\xf1ec")); // FontAwesome icon for calculator
        case ERiveDataType::String:
            return FString(TEXT("\xf031")); // FontAwesome icon for font
        case ERiveDataType::Trigger:
            return FString(TEXT("\xf0e7")); // FontAwesome icon for bolt
        case ERiveDataType::List:
            return FString(TEXT("\xf00b")); // FontAwesome icon for list
        case ERiveDataType::ViewModel:
            return FString(
                TEXT("\xf542")); // FontAwesome icon for Project Diagram
        default:
            return FString(TEXT("\xf128")); // FontAwesome icon for
                                            // question-circle (unknown type)
    }
}
static FString GetTextForPropertyType(ERiveDataType Type)
{
    switch (Type)
    {
        case ERiveDataType::Boolean:
            return FString(TEXT("Boolean"));
        case ERiveDataType::Color:
            return FString(TEXT("Color"));
        case ERiveDataType::EnumType:
            return FString(TEXT("Enum"));
        case ERiveDataType::Number:
            return FString(TEXT("Number"));
        case ERiveDataType::String:
            return FString(TEXT("String"));
        case ERiveDataType::Trigger:
            return FString(TEXT("Trigger"));
        case ERiveDataType::List:
            return FString(TEXT("List"));
        case ERiveDataType::ViewModel:
            return FString(TEXT("ViewModel"));
        case ERiveDataType::AssetImage:
            return FString(TEXT("Texture"));
        case ERiveDataType::Artboard:
            return FString(TEXT("Artboard"));
        case ERiveDataType::None:
            UE_LOG(
                LogRiveEditor,
                Warning,
                TEXT(
                    "Getting ERiveDataType Text value for \"None\" Which is likey not intended!"));
            return FString(TEXT("None"));
    }

    UE_LOG(LogRiveEditor,
           Warning,
           TEXT("GetTextForPropertyType \"Type\" is invalid."));
    return FString(TEXT("Invalid"));
}
} // namespace RiveArtboardDetailCustomizationPrivate

TSharedRef<IDetailCustomization> FRiveFileDetailCustomization::MakeInstance()
{
    return MakeShareable(new FRiveFileDetailCustomization);
}

void FRiveFileDetailCustomization::CustomizeDetails(
    IDetailLayoutBuilder& DetailBuilder)
{
    auto RiveFiles = DetailBuilder.GetSelectedObjectsOfType<URiveFile>();
    check(RiveFiles.Num());
    auto RiveFile = RiveFiles[0];

    IDetailCategoryBuilder& MyCategory = DetailBuilder.EditCategory("Rive");

    // Find the property you want to customize
    TSharedRef<IPropertyHandle> ArtboardDefinitionsProperty =
        DetailBuilder.GetProperty(
            GET_MEMBER_NAME_CHECKED(URiveFile, ArtboardDefinitions));

    TArray<FArtboardDefinition>& ArtboardDefinitions =
        RiveFile->ArtboardDefinitions;

    uint32 NumArtboardElements;
    ArtboardDefinitionsProperty->GetNumChildren(NumArtboardElements);
    check(NumArtboardElements == ArtboardDefinitions.Num());
    DetailBuilder.HideProperty(ArtboardDefinitionsProperty);

    auto& ArtboardsGroup =
        MyCategory.AddGroup(FName(TEXT("Artboards")),
                            FText::FromString(TEXT("Artboards")));

    for (uint32 Index = 0; Index < NumArtboardElements; Index++)
    {
        auto Property =
            ArtboardDefinitionsProperty->GetChildHandle(Index).ToSharedRef();
        auto ArtboardDefinition = ArtboardDefinitions[Index];

        auto& ArtboardGroup =
            ArtboardsGroup.AddGroup(FName(ArtboardDefinition.Name),
                                    FText::FromString(ArtboardDefinition.Name));

        auto DefaultViewModelProperty =
            Property->GetChildHandle(2).ToSharedRef();
        ArtboardGroup.AddPropertyRow(DefaultViewModelProperty);
        auto DefaultViewModelInstanceProperty =
            Property->GetChildHandle(3).ToSharedRef();
        ArtboardGroup.AddPropertyRow(DefaultViewModelInstanceProperty);

        auto& StateMachinesGroup =
            ArtboardGroup.AddGroup(FName("StateMachines"),
                                   FText::FromString(TEXT("StateMachines")));
        auto StateMachineNamesProperty =
            Property->GetChildHandle(1).ToSharedRef();
        uint32 NumStateMachineElements;
        StateMachineNamesProperty->GetNumChildren(NumStateMachineElements);
        check(NumStateMachineElements ==
              ArtboardDefinition.StateMachineNames.Num());

        for (uint32 StateMachineIndex = 0;
             StateMachineIndex < NumStateMachineElements;
             StateMachineIndex++)
        {
            auto StateMachineProperty =
                StateMachineNamesProperty->GetChildHandle(StateMachineIndex)
                    .ToSharedRef();
            auto StateMachineName =
                ArtboardDefinition.StateMachineNames[StateMachineIndex];

            StateMachinesGroup.AddPropertyRow(StateMachineProperty)
                .CustomWidget()
                .WholeRowContent()
                    [SNew(SHorizontalBox) +
                     SHorizontalBox::Slot()
                         .VAlign(VAlign_Center)
                         .Padding(5.f, 0.f, 0.f, 0.f)
                         .AutoWidth()[SNew(STextBlock)
                                          .Font(FAppStyle::Get().GetFontStyle(
                                              "FontAwesome.11"))
                                          .Text(FText::FromString(
                                              FString(TEXT("\xf0e8"))))
                                          .ToolTipText(FText::FromString(
                                              TEXT("StateMachine")))] +
                     SHorizontalBox::Slot()
                         .VAlign(VAlign_Center)
                         .Padding(5.f, 0.f)
                         .AutoWidth()[SNew(STextBlock)
                                          .Font(DEFAULT_FONT("Regular", 8))
                                          .Text(FText::FromString(
                                              StateMachineName))]];
        }
    }

    // Viewmodels
    TSharedRef<IPropertyHandle> ViewModels = DetailBuilder.GetProperty(
        GET_MEMBER_NAME_CHECKED(URiveFile, ViewModelDefinitions));
    TArray<FViewModelDefinition>& ViewModelDefinitions =
        RiveFile->ViewModelDefinitions;
    DetailBuilder.HideProperty(ViewModels);

    uint32 NumViewModels;
    ViewModels->GetNumChildren(NumViewModels);
    check(NumViewModels == ViewModelDefinitions.Num());

    auto& ViewModelsGroup =
        MyCategory.AddGroup(TEXT("View Models"),
                            FText::FromString(TEXT("View Models")));

    for (uint32 Index = 0; Index < NumViewModels; ++Index)
    {
        auto ViewModelDefinitionProperty =
            ViewModels->GetChildHandle(Index).ToSharedRef();
        auto& ViewModelDefinition = ViewModelDefinitions[Index];
        auto& ViewModelDefinitionGroup = ViewModelsGroup.AddGroup(
            FName(ViewModelDefinition.Name),
            FText::FromString(ViewModelDefinition.Name));

        // Instance Names
        TSharedRef<IPropertyHandle> InstanceNameHandle =
            ViewModelDefinitionProperty->GetChildHandle(1).ToSharedRef();
        uint32 NumInstanceNames;
        InstanceNameHandle->GetNumChildren(NumInstanceNames);
        check(ViewModelDefinition.InstanceNames.Num() == NumInstanceNames);

        auto& InstanceNamesGroup = ViewModelDefinitionGroup.AddGroup(
            TEXT("Instance Names"),
            FText::FromString(TEXT("Instance Names")));
        for (uint32 InstanceIndex = 0; InstanceIndex < NumInstanceNames;
             ++InstanceIndex)
        {
            auto Property =
                InstanceNameHandle->GetChildHandle(InstanceIndex).ToSharedRef();
            auto InstanceName =
                ViewModelDefinition.InstanceNames[InstanceIndex];
            InstanceNamesGroup.AddPropertyRow(Property)
                .CustomWidget()
                .WholeRowContent()
                    [SNew(SHorizontalBox) +
                     SHorizontalBox::Slot()
                         .VAlign(VAlign_Center)
                         .AutoWidth()[SNew(STextBlock)
                                          .Font(FAppStyle::Get().GetFontStyle(
                                              "FontAwesome.11"))
                                          .Text(FText::FromString(
                                              FString(TEXT("\xf0c5"))))
                                          .ToolTipText(FText::FromString(
                                              TEXT("Instance")))] +
                     SHorizontalBox::Slot()
                         .VAlign(VAlign_Center)
                         .Padding(10.f, 0.f)
                         .AutoWidth()[SNew(STextBlock)
                                          .Font(DEFAULT_FONT("Regular", 8))
                                          .Text(FText::FromString(
                                              InstanceName))]];
        }

        // Properties
        TSharedRef<IPropertyHandle> PropertiesHandle =
            ViewModelDefinitionProperty->GetChildHandle(2).ToSharedRef();
        auto& PropertiesGroup = ViewModelDefinitionGroup.AddGroup(
            TEXT("View Model Properties"),
            FText::FromString(TEXT("View Model Properties")));
        uint32 NumProperties;
        PropertiesHandle->GetNumChildren(NumProperties);
        check(NumProperties == ViewModelDefinition.PropertyDefinitions.Num());
        for (uint32 PropertiesIndex = 0; PropertiesIndex < NumProperties;
             ++PropertiesIndex)
        {
            auto Property =
                PropertiesHandle->GetChildHandle(PropertiesIndex).ToSharedRef();
            auto& PropertyDefinition =
                ViewModelDefinition.PropertyDefinitions[PropertiesIndex];
            PropertiesGroup.AddPropertyRow(Property)
                .CustomWidget()
                .WholeRowContent()
                    [SNew(SHorizontalBox) +
                     SHorizontalBox::Slot()
                         .VAlign(VAlign_Center)
                         .AutoWidth()
                             [SNew(STextBlock)
                                  .Font(FAppStyle::Get().GetFontStyle(
                                      "FontAwesome.11"))
                                  .Text(FText::FromString(
                                      RiveArtboardDetailCustomizationPrivate::
                                          GetIconForPropertyType(
                                              PropertyDefinition.Type)))
                                  .ToolTipText(FText::FromString(
                                      RiveArtboardDetailCustomizationPrivate::
                                          GetTextForPropertyType(
                                              PropertyDefinition.Type) +
                                      TEXT(" Property")))] +
                     SHorizontalBox::Slot()
                         .VAlign(VAlign_Center)
                         .Padding(10.f, 0.f)
                         .AutoWidth()[SNew(STextBlock)
                                          .Font(DEFAULT_FONT("Regular", 8))
                                          .Text(FText::FromString(
                                              PropertyDefinition.Name))]];
        }
    }

    // Enums
    TSharedRef<IPropertyHandle> EnumsProperty = DetailBuilder.GetProperty(
        GET_MEMBER_NAME_CHECKED(URiveFile, EnumDefinitions));
    TArray<FEnumDefinition>& EnumDefinitions = RiveFile->EnumDefinitions;

    uint32 NumEnums;
    EnumsProperty->GetNumChildren(NumEnums);
    check(NumEnums == EnumDefinitions.Num());

    TSharedRef<FDetailArrayBuilder> EnumsBuilder = MakeShareable(
        new FDetailArrayBuilder(EnumsProperty, true, false, false));
    EnumsBuilder->OnGenerateArrayElementWidget(
        FOnGenerateArrayElementWidget::CreateLambda([EnumDefinitions](
                                                        TSharedRef<
                                                            IPropertyHandle>
                                                            Property,
                                                        int32 Index,
                                                        IDetailChildrenBuilder&
                                                            Builder) {
            auto EnumDefinition = EnumDefinitions[Index];
            auto EnumValuesProperty = Property->GetChildHandle(1).ToSharedRef();
            TSharedRef<FDetailArrayBuilder> EnumsValuesBuilder =
                MakeShareable(new FDetailArrayBuilder(EnumValuesProperty,
                                                      true,
                                                      false,
                                                      false));
            EnumsValuesBuilder->SetDisplayName(
                FText::FromString(EnumDefinition.Name));
            EnumsValuesBuilder->OnGenerateArrayElementWidget(
                FOnGenerateArrayElementWidget::CreateLambda(
                    [EnumDefinition](TSharedRef<IPropertyHandle> Property,
                                     int32 Index,
                                     IDetailChildrenBuilder& Builder) {
                        auto EnumValue = EnumDefinition.Values[Index];
                        Builder.AddProperty(Property)
                            .CustomWidget()
                            .WholeRowContent()
                                [SNew(SHorizontalBox) +
                                 SHorizontalBox::Slot()
                                     .VAlign(VAlign_Center)
                                     .AutoWidth()
                                         [SNew(STextBlock)
                                              .Font(
                                                  FAppStyle::Get().GetFontStyle(
                                                      "FontAwesome.11"))
                                              .Text(FText::FromString(
                                                  FString(TEXT("\xf068"))))
                                              .ToolTipText(FText::FromString(
                                                  TEXT("Enum Name")))] +
                                 SHorizontalBox::Slot()
                                     .VAlign(VAlign_Center)
                                     .Padding(10.f, 0.f)
                                     .AutoWidth()
                                         [SNew(STextBlock)
                                              .Font(DEFAULT_FONT("Regular", 8))
                                              .Text(FText::FromString(
                                                  EnumValue))]];
                    }));

            Builder.AddCustomBuilder(EnumsValuesBuilder);
        }));

    MyCategory.AddCustomBuilder(EnumsBuilder);
}

#undef LOCTEXT_NAMESPACE