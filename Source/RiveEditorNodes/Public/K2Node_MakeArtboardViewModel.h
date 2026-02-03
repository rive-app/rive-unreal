// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_MakeViewModelBase.h"
#include "K2Node_MakeArtboardViewModel.generated.h"

/**
 *
 */
UCLASS()
class RIVEEDITORNODES_API UK2Node_MakeArtboardViewModel
    : public UK2Node_MakeViewModelBase
{
    GENERATED_BODY()
public:
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

    virtual FText GetTooltipText() const override;
    virtual FText GetToolTipHeading() const override;
    virtual FText GetKeywords() const override;

    virtual bool CheckForErrors(
        FKismetCompilerContext& CompilerContext) override;

protected:
    virtual UEnum* GenerateViewModelSourceEnum(
        TObjectPtr<class URiveFile> RiveFile) override;
    virtual UEdGraphPin* GetTargetPinForViewModelSource(
        const UK2Node* Node) override;
    virtual int GetIndexFromSelection(const FString& Selection) override;
    virtual FViewModelDefinition* GetViewModelDefinitionFromIndex(
        int Index) override;
    virtual FText GetViewModelInputName() override
    {
        return FText::FromString("Select Artboard");
    }
    virtual void SetFunctionOnIntermediateNode(
        UK2Node_CallFunction* Node) const override;

private:
    FNodeTextCache NodeTitleCache;
};
