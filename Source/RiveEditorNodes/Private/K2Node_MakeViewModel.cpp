// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "K2Node_MakeViewModel.h"

#include "UObject/ReflectedTypeAccessors.h"
#include "K2Node_CallFunction.h"
#include "EdGraphSchema_K2.h"
#include "KismetCompiler.h"
#include "Rive/RiveFile.h"

#define LOCTEXT_NAMESPACE "CreateViewModelNode"

FText UK2Node_MakeViewModel::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    if (NodeTitleCache.IsOutOfDate(this))
    {
        NodeTitleCache.SetCachedText(
            NSLOCTEXT("Rive", "NodeTitle", "Make View Model"),
            this);
    }
    return NodeTitleCache;
}

FText UK2Node_MakeViewModel::GetTooltipText() const
{
    return FText::FromString(
        "Used to create Rive View Model.\nTo use, select a file in the "
        "dropdown to create the view model from and then select the view model "
        "type and the instance name.");
}

FText UK2Node_MakeViewModel::GetToolTipHeading() const
{
    return FText::FromString("Make View Model");
}

FText UK2Node_MakeViewModel::GetKeywords() const
{
    return FText::FromString("Rive View Model");
}

bool UK2Node_MakeViewModel::CheckForErrors(
    FKismetCompilerContext& CompilerContext)
{
    auto RiveFile = GetSelectedRiveFile();

    if (!IsValid(RiveFile))
    {
        return Super::CheckForErrors(CompilerContext);
    }

    if (!RiveFile->GetHasData())
    {
        return Super::CheckForErrors(CompilerContext);
    }

    auto ViewModelSource = GetSelectedViewModelSource();

    if (ViewModelSource.IsEmpty())
    {
        CompilerContext.MessageLog.Error(
            *FString::Printf(TEXT("Selected view model is invalid.")));
        return true;
    }

    if (RiveFile->ViewModelDefinitions.FindByPredicate(
            [ViewModelSource](const FViewModelDefinition& L) {
                return L.Name == ViewModelSource;
            }) == nullptr)
    {
        CompilerContext.MessageLog.Error(*FString::Printf(
            TEXT("Selected view model is not in selected rive file.")));
        return true;
    }

    return Super::CheckForErrors(CompilerContext);
}

UEnum* UK2Node_MakeViewModel::GenerateViewModelSourceEnum(
    TObjectPtr<URiveFile> RiveFile)
{
    check(RiveFile->ViewModelEnum);
    return RiveFile->ViewModelEnum;
}

int UK2Node_MakeViewModel::GetIndexFromSelection(const FString& Selection)
{
    auto RiveFile = GetSelectedRiveFile();
    check(RiveFile);
    for (int Index = 0; Index < RiveFile->ViewModelDefinitions.Num(); ++Index)
    {
        if (RiveFile->ViewModelDefinitions[Index].Name == Selection)
        {
            return Index;
        }
    }

    return Super::GetIndexFromSelection(Selection);
}

UEdGraphPin* UK2Node_MakeViewModel::GetTargetPinForViewModelSource(
    const UK2Node* Node)
{
    return Node->FindPin(TEXT("ViewModelName"));
}

FViewModelDefinition* UK2Node_MakeViewModel::GetViewModelDefinitionFromIndex(
    int Index)
{
    auto RiveFile = GetSelectedRiveFile();
    check(RiveFile);
    check(RiveFile->ViewModelDefinitions.IsValidIndex(Index));
    return &RiveFile->ViewModelDefinitions[Index];
}

void UK2Node_MakeViewModel::SetFunctionOnIntermediateNode(
    UK2Node_CallFunction* Node) const
{
    static const FName FunctionName =
        GET_FUNCTION_NAME_CHECKED(URiveFile, CreateViewModelByName);
    Node->FunctionReference.SetExternalMember(FunctionName,
                                              URiveFile::StaticClass());
}

#undef LOCTEXT_NAMESPACE