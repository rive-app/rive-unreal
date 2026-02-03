// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "Rive/RiveFile.h"
#include "K2Node_MakeViewModelBase.generated.h"

struct FViewModelDefinition;
/**
 *
 */
UCLASS(Abstract)
class UK2Node_MakeViewModelBase : public UK2Node
{
    GENERATED_BODY()
public:
    virtual void AllocateDefaultPins() override;
    virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;

    virtual void ReallocatePinsDuringReconstruction(
        TArray<UEdGraphPin*>& OldPins) override;

    virtual void ExpandNode(FKismetCompilerContext& CompilerContext,
                            UEdGraph* SourceGraph) override;
    virtual bool CheckForErrors(FKismetCompilerContext& CompilerContext);

    virtual bool IsNodeSafeToIgnore() const override { return true; }
    virtual void GetMenuActions(
        FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
    virtual FText GetMenuCategory() const override;

protected:
    virtual void BeginDestroy() override;

    virtual UEnum* GenerateViewModelSourceEnum(
        TObjectPtr<class URiveFile> RiveFile)
    {
        return nullptr;
    }
    virtual int GetIndexFromSelection(const FString& Selection);
    virtual FViewModelDefinition* GetViewModelDefinitionFromIndex(int Index)
    {
        return nullptr;
    }

    virtual void SetFunctionOnIntermediateNode(
        class UK2Node_CallFunction* Node) const
    {}

    virtual FText GetViewModelInputName() { return FText::FromString(""); }
    virtual FText GetViewModelInstanceInputName()
    {
        return FText::FromString("Select Instance Name");
    }

    virtual UEdGraphPin* GetTargetPinForViewModelSource(const UK2Node* Node)
    {
        return nullptr;
    }

    TObjectPtr<URiveFile> GetSelectedRiveFile() const
    {
        return SelectedRiveFile;
    }

    FString GetSelectedViewModelSource() const
    {
        if (auto ViewModelSourcePin = GetViewModelSourcePin())
        {
            return ViewModelSourcePin->GetDefaultAsString();
        }
        return "";
    }

private:
    void GenerateViewModelSourcePin(TObjectPtr<class URiveFile> RiveFile);
    void GenerateViewModelInstanceEnumPin(
        const struct FViewModelDefinition& SelectedViewModel);
    void UpdatePins(UEdGraphPin* UpdatedPin);

    UEdGraphPin* GetFilePin() const
    {
        UEdGraphPin* Pin = FindPin(PN_File);
        check(Pin != nullptr && Pin->Direction == EGPD_Input);
        return Pin;
    }

    UEdGraphPin* GetViewModelSourcePin() const
    {
        return FindPin(PN_ViewModelSource);
    }

    UEdGraphPin* GetViewModelInstancePin() const
    {
        return FindPin(PN_ViewModelInstance);
    }

    UEdGraphPin* GetReturnPin() const;

    UPROPERTY()
    TObjectPtr<URiveFile> SelectedRiveFile;

    UPROPERTY()
    int32 SelectedIndex = INDEX_NONE;

    UPROPERTY()
    TObjectPtr<UClass> ViewModelClass = nullptr;

    static FName PN_File;
    static FName PN_ViewModelSource;
    static FName PN_ViewModelInstance;
};
