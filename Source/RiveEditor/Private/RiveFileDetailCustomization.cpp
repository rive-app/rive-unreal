// Copyright Rive, Inc. All rights reserved.
#include "RiveFileDetailCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Logs/RiveEditorLog.h"
#include "Rive/RiveArtboard.h"
#include "Rive/RiveFile.h"
#include "Rive/ViewModel/RiveViewModel.h"
#include "Rive/viewmodel/viewmodel_property_string.hpp"
#include "Styling/SlateStyleMacros.h"
#include <string>

#include "EditorFontGlyphs.h"

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

static FString GetIconForPropertyType(rive::DataType Type)
{
    switch (Type)
    {
        case rive::DataType::boolean:
            return FString(TEXT("\xf046")); // FontAwesome icon for check-square
        case rive::DataType::color:
            return FString(TEXT("\xf53f")); // FontAwesome icon for palette
        case rive::DataType::enumType:
            return FString(TEXT("\xf0cb")); // FontAwesome icon for list
        case rive::DataType::number:
            return FString(TEXT("\xf1ec")); // FontAwesome icon for calculator
        case rive::DataType::string:
            return FString(TEXT("\xf031")); // FontAwesome icon for font
        case rive::DataType::trigger:
            return FString(TEXT("\xf0e7")); // FontAwesome icon for bolt
        case rive::DataType::list:
            return FString(TEXT("\xf00b")); // FontAwesome icon for list
        case rive::DataType::viewModel:
            return FString(
                TEXT("\xf542")); // FontAwesome icon for Project Diagram
        default:
            return FString(TEXT("\xf128")); // FontAwesome icon for
                                            // question-circle (unknown type)
    }
}
static FString GetTextForPropertyType(rive::DataType Type)
{
    switch (Type)
    {
        case rive::DataType::boolean:
            return FString(TEXT("Boolean"));
        case rive::DataType::color:
            return FString(TEXT("Color"));
        case rive::DataType::enumType:
            return FString(TEXT("Enum"));
        case rive::DataType::number:
            return FString(TEXT("Number"));
        case rive::DataType::string:
            return FString(TEXT("String"));
        case rive::DataType::trigger:
            return FString(TEXT("Trigger"));
        case rive::DataType::list:
            return FString(TEXT("List"));
        case rive::DataType::viewModel:
            return FString(TEXT("ViewModel"));
        default:
            RIVE_UNREACHABLE();
    }
}
} // namespace RiveArtboardDetailCustomizationPrivate

TSharedRef<IDetailCustomization> FRiveFileDetailCustomization::MakeInstance()
{
    return MakeShareable(new FRiveFileDetailCustomization);
}

void FRiveFileDetailCustomization::CustomizeDetails(
    IDetailLayoutBuilder& DetailBuilder)
{

    // Find the property you want to customize
    TSharedRef<IPropertyHandle> ArtboardsProperty = DetailBuilder.GetProperty(
        GET_MEMBER_NAME_CHECKED(URiveFile, Artboards));

    IDetailCategoryBuilder& MyCategory = DetailBuilder.EditCategory("Rive");

    const TAttribute<bool> EditCondition =
        TAttribute<bool>::Create([this]() { return false; });

    MyCategory.AddProperty(ArtboardsProperty)
        .CustomWidget(false)
        .WholeRowContent()[SNew(SHorizontalBox) +
                           SHorizontalBox::Slot().AutoWidth().Padding(
                               0.f,
                               5.f)[SNew(STextBlock)
                                        .Font(DEFAULT_FONT("Regular", 8))
                                        .Text(FText::FromString("Artboards"))]];

    uint32 NumElements;
    ArtboardsProperty->GetNumChildren(NumElements);

    for (uint32 Index = 0; Index < NumElements; ++Index)
    {
        TSharedRef<IPropertyHandle> ElementHandle =
            ArtboardsProperty->GetChildHandle(Index).ToSharedRef();

        UObject* Object = nullptr;
        if (ElementHandle->GetValue(Object) == FPropertyAccess::Success)
        {
            if (URiveArtboard* Artboard = Cast<URiveArtboard>(Object))
            {
                MyCategory.AddProperty(ElementHandle)
                    .CustomWidget()
                    .WholeRowContent()
                        [SNew(SHorizontalBox) +
                         SHorizontalBox::Slot()
                             .VAlign(VAlign_Center)
                             .Padding(10.f, 0.f, 0.f, 0.f)
                             .AutoWidth()[SNew(STextBlock)
                                              .Font(
                                                  FAppStyle::Get().GetFontStyle(
                                                      "FontAwesome.11"))
                                              .Text(FText::FromString(
                                                  FString(TEXT("\xf247"))))
                                              .ToolTipText(FText::FromString(
                                                  TEXT("Artboard")))] +
                         SHorizontalBox::Slot()
                             .VAlign(VAlign_Center)
                             .Padding(10.f, 0.f)
                             .AutoWidth()[SNew(SEditableTextBox)
                                              .IsReadOnly(true)
                                              .Font(DEFAULT_FONT("Mono", 8))
                                              .Text(FText::FromString(
                                                  Artboard
                                                      ->GetArtboardName()))]];

                for (auto StateMachineName : Artboard->GetStateMachineNames())
                {
                    TUniquePtr<FRiveStateMachine> StateMachine =
                        MakeUnique<FRiveStateMachine>(
                            Artboard->GetNativeArtboard(),
                            StateMachineName);
                    uint32 StateMachineInputCount =
                        StateMachine->GetInputCount();

                    MyCategory.AddProperty(ElementHandle)
                        .CustomWidget()
                        .WholeRowContent()
                            [SNew(SHorizontalBox) +
                             SHorizontalBox::Slot()
                                 .VAlign(VAlign_Center)
                                 .Padding(20.f, 0.f, 0.f, 0.f)
                                 .AutoWidth()
                                     [SNew(STextBlock)
                                          .Font(FAppStyle::Get().GetFontStyle(
                                              "FontAwesome.11"))
                                          .Text(FText::FromString(
                                              FString(TEXT("\xf0e8"))))
                                          .ToolTipText(FText::FromString(
                                              TEXT("StateMachine")))] +
                             SHorizontalBox::Slot()
                                 .VAlign(VAlign_Center)
                                 .Padding(10.f, 0.f)
                                 .AutoWidth()[SNew(SEditableTextBox)
                                                  .IsReadOnly(true)
                                                  .Font(DEFAULT_FONT("Mono", 8))
                                                  .Text(FText::FromString(
                                                      StateMachineName))]];

                    for (uint32 InputI = 0; InputI < StateMachineInputCount;
                         InputI++)
                    {
                        rive::SMIInput* Input = StateMachine->GetInput(InputI);
                        FString TypeString =
                            RiveArtboardDetailCustomizationPrivate::
                                GetTypeStringForInput(Input);
                        FLinearColor TypeColor =
                            RiveArtboardDetailCustomizationPrivate::
                                GetColorForInput(Input);
                        // std::string InputName = Input->name();

                        FString InputName(UTF8_TO_TCHAR(Input->name().c_str()));
                        MyCategory.AddProperty(ElementHandle)
                            .CustomWidget()
                            .WholeRowContent()
                                [SNew(SHorizontalBox) +
                                 SHorizontalBox::Slot()
                                     .VAlign(VAlign_Center)
                                     .Padding(30.f, 0.f, 0.f, 0.f)
                                     .AutoWidth()
                                         [SNew(SImage)
                                              .ColorAndOpacity(TypeColor)
                                              .Image(FAppStyle::GetBrush(
                                                  "Kismet.VariableList."
                                                  "TypeIcon"))] +
                                 SHorizontalBox::Slot()
                                     .VAlign(VAlign_Center)
                                     .Padding(5.f, 0.f)
                                     .AutoWidth()
                                         [SNew(STextBlock)
                                              .Font(DEFAULT_FONT("Mono", 8))
                                              .ColorAndOpacity(TypeColor)
                                              .Text(FText::FromString(
                                                  TypeString)) // FText::FromString(ElementHandle->GetProperty()
                        ] +
                                 SHorizontalBox::Slot()
                                     .VAlign(VAlign_Center)
                                     .Padding(5.f, 0.f)
                                     .AutoWidth()
                                         [SNew(SEditableTextBox)
                                              .IsReadOnly(true)
                                              .Font(DEFAULT_FONT("Mono", 8))
                                              .Text(FText::FromString(
                                                  InputName)) // FText::FromString(ElementHandle->GetProperty()
                        ]];
                    }
                }
            }
        }
    }

    // Viewmodels
    TSharedRef<IPropertyHandle> ViewModelsProperty = DetailBuilder.GetProperty(
        GET_MEMBER_NAME_CHECKED(URiveFile, ViewModels));

    MyCategory.AddProperty(ViewModelsProperty)
        .CustomWidget(false)
        .WholeRowContent()[SNew(SHorizontalBox) +
                           SHorizontalBox::Slot().AutoWidth().Padding(0.f, 5.f)
                               [SNew(STextBlock)
                                    .Font(DEFAULT_FONT("Regular", 8))
                                    .Text(FText::FromString("ViewModels"))]];

    ViewModelsProperty->GetNumChildren(NumElements);

    for (uint32 Index = 0; Index < NumElements; ++Index)
    {
        TSharedRef<IPropertyHandle> ElementHandle =
            ViewModelsProperty->GetChildHandle(Index).ToSharedRef();

        UObject* Object = nullptr;
        if (ElementHandle->GetValue(Object) == FPropertyAccess::Success)
        {
            if (URiveViewModel* ViewModel = Cast<URiveViewModel>(Object))
            {
                MyCategory.AddProperty(ElementHandle)
                    .CustomWidget()
                    .WholeRowContent()
                        [SNew(SHorizontalBox) +
                         SHorizontalBox::Slot()
                             .VAlign(VAlign_Center)
                             .Padding(10.f, 0.f, 0.f, 0.f)
                             .AutoWidth()[SNew(STextBlock)
                                              .Font(
                                                  FAppStyle::Get().GetFontStyle(
                                                      "FontAwesome.11"))
                                              .Text(FText::FromString(
                                                  FString(TEXT("\xf1c0"))))
                                              .ToolTipText(FText::FromString(
                                                  TEXT("ViewModel")))] +
                         SHorizontalBox::Slot()
                             .VAlign(VAlign_Center)
                             .Padding(10.f, 0.f)
                             .AutoWidth()[SNew(SEditableTextBox)
                                              .IsReadOnly(true)
                                              .Font(DEFAULT_FONT("Mono", 8))
                                              .Text(FText::FromString(
                                                  ViewModel->GetName()))]];

                // Instances
                MyCategory.AddProperty(ViewModelsProperty)
                    .CustomWidget(false)
                    .WholeRowContent()
                        [SNew(SHorizontalBox) +
                         SHorizontalBox::Slot()
                             .VAlign(VAlign_Center)
                             .AutoWidth()
                             .Padding(20.f, 0.f, 0.f, 0.f)
                                 [SNew(STextBlock)
                                      .Font(DEFAULT_FONT("Regular", 8))
                                      .Text(FText::FromString("Instances"))]];
                for (auto InstanceName : ViewModel->GetInstanceNames())
                {
                    MyCategory.AddProperty(ElementHandle)
                        .CustomWidget()
                        .WholeRowContent()
                            [SNew(SHorizontalBox) +
                             SHorizontalBox::Slot()
                                 .VAlign(VAlign_Center)
                                 .Padding(40.f, 0.f, 0.f, 0.f)
                                 .AutoWidth()
                                     [SNew(STextBlock)
                                          .Font(FAppStyle::Get().GetFontStyle(
                                              "FontAwesome.11"))
                                          .Text(FText::FromString(
                                              FString(TEXT("\xf0c5"))))
                                          .ToolTipText(FText::FromString(
                                              TEXT("Instance")))] +
                             SHorizontalBox::Slot()
                                 .VAlign(VAlign_Center)
                                 .Padding(10.f, 0.f)
                                 .AutoWidth()[SNew(SEditableTextBox)
                                                  .IsReadOnly(true)
                                                  .Font(DEFAULT_FONT("Mono", 8))
                                                  .Text(FText::FromString(
                                                      InstanceName))]];
                }

                // Properties
                MyCategory.AddProperty(ViewModelsProperty)
                    .CustomWidget(false)
                    .WholeRowContent()
                        [SNew(SHorizontalBox) +
                         SHorizontalBox::Slot()
                             .VAlign(VAlign_Center)
                             .AutoWidth()
                             .Padding(20.f, 0.f, 0.f, 0.f)
                                 [SNew(STextBlock)
                                      .Font(DEFAULT_FONT("Regular", 8))
                                      .Text(FText::FromString("Properties"))]];
                for (auto PropertyName : ViewModel->GetPropertyNames())
                {
                    auto Type = ViewModel->GetPropertyTypeByName(PropertyName);

                    MyCategory.AddProperty(ElementHandle)
                        .CustomWidget()
                        .WholeRowContent()
                            [SNew(SHorizontalBox) +
                             SHorizontalBox::Slot()
                                 .VAlign(VAlign_Center)
                                 .Padding(40.f, 0.f, 0.f, 0.f)
                                 .AutoWidth()
                                     [SNew(STextBlock)
                                          .Font(FAppStyle::Get().GetFontStyle(
                                              "FontAwesome.11"))
                                          .Text(FText::FromString(
                                              RiveArtboardDetailCustomizationPrivate::
                                                  GetIconForPropertyType(Type)))
                                          .ToolTipText(FText::FromString(
                                              RiveArtboardDetailCustomizationPrivate::
                                                  GetTextForPropertyType(Type) +
                                              TEXT(" Property")))] +
                             SHorizontalBox::Slot()
                                 .VAlign(VAlign_Center)
                                 .Padding(10.f, 0.f)
                                 .AutoWidth()[SNew(SEditableTextBox)
                                                  .IsReadOnly(true)
                                                  .Font(DEFAULT_FONT("Mono", 8))
                                                  .Text(FText::FromString(
                                                      PropertyName))]];
                }
            }
        }
    }
}

#undef LOCTEXT_NAMESPACE