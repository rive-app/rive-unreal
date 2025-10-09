#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_MakeArtboard.generated.h"

class URiveFile;

/**
 *
 */
UCLASS()
class RIVEEDITORNODES_API UK2Node_MakeArtboard : public UK2Node
{
    GENERATED_BODY()
public:
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FText GetTooltipText() const override;
    virtual FText GetToolTipHeading() const override;
    virtual FText GetKeywords() const override;

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

private:
    TObjectPtr<URiveFile> GetSelectedRiveFile() const
    {
        return SelectedRiveFile;
    }

    FString GetSelectedArtboard() const
    {
        if (auto ArtboardPin = GetArtboardSourcePin())
        {
            return ArtboardPin->GetDefaultAsString();
        }
        return "";
    }

    void UpdatePins(UEdGraphPin* UpdatedPin);

    UEdGraphPin* GetFilePin() const
    {
        UEdGraphPin* Pin = FindPin(PN_File);
        check(Pin != nullptr && Pin->Direction == EGPD_Input);
        return Pin;
    }

    UEdGraphPin* GetArtboardSourcePin() const
    {
        return FindPin(PN_ArtboardSource);
    }

    UEdGraphPin* GetAutoBindPin() const { return FindPin(PN_AutoBind); }

    UEdGraphPin* GetReturnPin() const;

    UPROPERTY()
    TObjectPtr<URiveFile> SelectedRiveFile;

    static FName PN_File;
    static FName PN_ArtboardSource;
    static FName PN_AutoBind;

    FNodeTextCache NodeTitleCache;
};
