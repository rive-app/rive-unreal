// Fill out your copyright notice in the Description page of Project Settings.

#include "K2Node_MakeArtboardViewModel.h"
#include "K2Node_CallFunction.h"
#include "UObject/ReflectedTypeAccessors.h"
#include "EdGraphSchema_K2.h"
#include "KismetCompiler.h"
#include "Logs/LogRiveNode.h"
#include "Rive/RiveFile.h"

FText UK2Node_MakeArtboardViewModel::GetNodeTitle(
    ENodeTitleType::Type TitleType) const
{
    if (NodeTitleCache.IsOutOfDate(this))
    {
        NodeTitleCache.SetCachedText(
            NSLOCTEXT("Rive", "NodeTitle", "Make Artboard View Model"),
            this);
    }
    return NodeTitleCache;
}

FText UK2Node_MakeArtboardViewModel::GetTooltipText() const
{
    return FText::FromString(
        "Used to create a default view model for a given artboard. To use, "
        "select a file in the dropdown and then select the artboard.");
}

FText UK2Node_MakeArtboardViewModel::GetToolTipHeading() const
{
    return FText::FromString("Make Artboard View Model");
}

FText UK2Node_MakeArtboardViewModel::GetKeywords() const
{
    return FText::FromString("Rive Artboard View Model");
}

bool UK2Node_MakeArtboardViewModel::CheckForErrors(
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
            *FString::Printf(TEXT("Selected artboard is invalid.")));
        return true;
    }

    auto SelectedArtboard = RiveFile->ArtboardDefinitions.FindByPredicate(
        [ViewModelSource](const FArtboardDefinition& L) {
            return L.Name == ViewModelSource;
        });
    if (SelectedArtboard == nullptr)
    {
        CompilerContext.MessageLog.Error(*FString::Printf(
            TEXT("Selected artboard is not in selected rive file.")));
        return true;
    }

    if (SelectedArtboard->DefaultViewModel.IsEmpty())
    {
        CompilerContext.MessageLog.Error(*FString::Printf(
            TEXT("Selected artboard %s does not contain a default view model."),
            *SelectedArtboard->Name));
        return true;
    }

    return Super::CheckForErrors(CompilerContext);
}

UEnum* UK2Node_MakeArtboardViewModel::GenerateViewModelSourceEnum(
    TObjectPtr<URiveFile> RiveFile)
{
    check(RiveFile->ArtboardEnum);
    return RiveFile->ArtboardEnum;
}

UEdGraphPin* UK2Node_MakeArtboardViewModel::GetTargetPinForViewModelSource(
    const UK2Node* Node)
{
    return Node->FindPin(TEXT("ArtboardName"));
}

int UK2Node_MakeArtboardViewModel::GetIndexFromSelection(
    const FString& Selection)
{
    auto RiveFile = GetSelectedRiveFile();
    check(RiveFile);
    for (int Index = 0; Index < RiveFile->ArtboardDefinitions.Num(); ++Index)
    {
        if (RiveFile->ArtboardDefinitions[Index].Name == Selection)
        {
            return Index;
        }
    }

    return Super::GetIndexFromSelection(Selection);
}

FViewModelDefinition* UK2Node_MakeArtboardViewModel::
    GetViewModelDefinitionFromIndex(int Index)
{
    auto RiveFile = GetSelectedRiveFile();
    check(RiveFile);
    check(RiveFile->ArtboardDefinitions.IsValidIndex(Index));
    auto ArtboardDefinition = RiveFile->ArtboardDefinitions[Index];
    auto ViewModelName = ArtboardDefinition.DefaultViewModel;
    for (int ViewModelIndex = 0;
         ViewModelIndex < RiveFile->ViewModelDefinitions.Num();
         ++ViewModelIndex)
    {
        if (RiveFile->ViewModelDefinitions[ViewModelIndex].Name ==
            ViewModelName)
        {
            return &RiveFile->ViewModelDefinitions[ViewModelIndex];
        }
    }
    UE_LOG(LogRiveNode,
           Error,
           TEXT("Can not find default view model %s for Artboard %s "),
           *ArtboardDefinition.DefaultViewModel,
           *ArtboardDefinition.Name);
    return nullptr;
}

void UK2Node_MakeArtboardViewModel::SetFunctionOnIntermediateNode(
    UK2Node_CallFunction* Node) const
{
    static const FName FunctionName =
        GET_FUNCTION_NAME_CHECKED(URiveFile, CreateViewModelByArtboardName);
    Node->FunctionReference.SetExternalMember(FunctionName,
                                              URiveFile::StaticClass());
}

#undef LOCTEXT_NAMESPACE
