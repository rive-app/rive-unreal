// Copyright Rive, Inc. All rights reserved.
#include "RiveFileDetailCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Logs/RiveEditorLog.h"
#include "Rive/RiveArtboard.h"
#include "Rive/RiveFile.h"
#include "rive/animation/state_machine_input_instance.hpp"
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
} // namespace RiveArtboardDetailCustomizationPrivate

TSharedRef<IDetailCustomization> FRiveFileDetailCustomization::MakeInstance()
{
    return MakeShareable(new FRiveFileDetailCustomization);
}

void FRiveFileDetailCustomization::CustomizeDetails(
    IDetailLayoutBuilder& DetailBuilder)
{

    // Find the property you want to customize
    TSharedRef<IPropertyHandle> MyProperty = DetailBuilder.GetProperty(
        GET_MEMBER_NAME_CHECKED(URiveFile, Artboards));

    IDetailCategoryBuilder& MyCategory = DetailBuilder.EditCategory("Rive");

    const TAttribute<bool> EditCondition =
        TAttribute<bool>::Create([this]() { return false; });

    MyCategory.AddProperty(MyProperty)
        .CustomWidget(false)
        .WholeRowContent()[SNew(SHorizontalBox) +
                           SHorizontalBox::Slot().AutoWidth().Padding(
                               0.f,
                               5.f)[SNew(STextBlock)
                                        .Font(DEFAULT_FONT("Regular", 8))
                                        .Text(FText::FromString("Artboards"))]];

    uint32 NumElements;
    MyProperty->GetNumChildren(NumElements);

    for (uint32 Index = 0; Index < NumElements; ++Index)
    {
        TSharedRef<IPropertyHandle> ElementHandle =
            MyProperty->GetChildHandle(Index).ToSharedRef();

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
                                [SNew(SHorizontalBox)
                                 // +SHorizontalBox::Slot()
                                 // 	.VAlign(VAlign_Center)
                                 // 	.Padding(20.f, 0.f, 0.f, 0.f)
                                 // 	.AutoWidth()
                                 // 	[
                                 // 	SNew(STextBlock)
                                 // 		.Font(FAppStyle::Get().GetFontStyle("FontAwesome.11"))
                                 // 		.Text(FText::FromString(FString(TEXT("\xf11c"))))//FText::FromString(ElementHandle->GetProperty()
                                 //
                                 // 	]
                                 + SHorizontalBox::Slot()
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

    // .MinDesiredWidth(200)
    // // .MaxDesiredWidth(600)
    // [
    // 	SNew(STextBlock)
    // 	.Text(FText::FromString(TEXT("Hi!!!")))
    // //FText::FromString(ElementHandle->GetProperty()
    // // 	SNew(SHorizontalBox)
    // // 	+ SHorizontalBox::Slot()
    // // 	.AutoWidth()
    // // 	// [
    // // 	// 	// MyProperty->CreatePropertyValueWidget()
    // // 	// ]
    // // 	+ SHorizontalBox::Slot()
    // // 	.AutoWidth()
    // // 	.Padding(5, 0, 0, 0)
    // // 	[
    // // 		SNew(SButton)
    // // 		.Text(FText::FromString("Custom Button"))
    // // 		.OnClicked(FOnClicked::CreateLambda([]() -> FReply {
    // // 			// Handle button click here
    // // 			return FReply::Handled();
    // // 		}))
    // // 	]
    // ];

    // Find the array property you want to customize
    // TSharedRef<IPropertyHandle> MyArrayProperty =
    // DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(URiveFile, Artboards));

    //
    // // Create a custom widget for each element in the array
    // uint32 NumElements;
    // MyArrayProperty->GetNumChildren(NumElements);
    //
    // IDetailCategoryBuilder& MyCategory = DetailBuilder.EditCategory("Rive");
    //
    // MyArrayProperty->IsCustomized()
    //
    // for (uint32 Index = 0; Index < NumElements; ++Index)
    // {
    // 	TSharedRef<IPropertyHandle> ElementHandle =
    // MyArrayProperty->GetChildHandle(Index).ToSharedRef();
    //
    //
    //
    //
    // 	MyCategory.AddProperty(ElementHandle)
    // 		.CustomWidget()
    // 		.NameContent()
    // 		[
    // 			ElementHandle->CreatePropertyNameWidget()
    // 		]
    // 		.ValueContent()
    // 		[
    // 			SNew(STextBlock)
    // 			.Text(FText::FromString(TEXT("Hi!!!")))
    // //FText::FromString(ElementHandle->GetProperty()
    // 		];
    // }
}

#undef LOCTEXT_NAMESPACE