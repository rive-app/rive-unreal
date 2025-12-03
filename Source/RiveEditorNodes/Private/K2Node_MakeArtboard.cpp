// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "K2Node_MakeArtboard.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "KismetCompiler.h"
#include "Logs/LogRiveNode.h"
#include "Rive/RiveArtboard.h"
#include "Rive/RiveFile.h"
#include "Rive/RiveViewModel.h"
#include "PinTools.h"

FName UK2Node_MakeArtboard::PN_File = TEXT("FileInput");
FName UK2Node_MakeArtboard::PN_ArtboardSource = TEXT("ArtboardSourceInput");
FName UK2Node_MakeArtboard::PN_AutoBind = TEXT("AutoBind");

FText UK2Node_MakeArtboard::GetMenuCategory() const
{
    return FText::FromString("Rive");
}

void UK2Node_MakeArtboard::GetMenuActions(
    FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    Super::GetMenuActions(ActionRegistrar);

    UClass* ActionKey = GetClass();
    if (ActionRegistrar.IsOpenForRegistration(ActionKey))
    {
        UBlueprintNodeSpawner* NodeSpawner =
            UBlueprintNodeSpawner::Create(GetClass());
        check(NodeSpawner);

        ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
    }
}

UEdGraphPin* UK2Node_MakeArtboard::GetReturnPin() const
{
    UEdGraphPin* Pin = FindPin(UEdGraphSchema_K2::PN_ReturnValue);
    check(Pin != nullptr && Pin->Direction == EGPD_Output);
    return Pin;
}

FText UK2Node_MakeArtboard::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    if (NodeTitleCache.IsOutOfDate(this))
    {
        NodeTitleCache.SetCachedText(
            NSLOCTEXT("Rive", "NodeTitle", "Make Artboard"),
            this);
    }
    return NodeTitleCache;
}

FText UK2Node_MakeArtboard::GetTooltipText() const
{
    return FText::FromString(
        "Used to create artboards.\nTo use, select a file in the "
        "dropdown to create the artboard from and then select the artboard "
        "name.");
}

FText UK2Node_MakeArtboard::GetToolTipHeading() const
{
    return FText::FromString("Make Artboard");
}

FText UK2Node_MakeArtboard::GetKeywords() const
{
    return FText::FromString("Rive Artboard");
}

void UK2Node_MakeArtboard::AllocateDefaultPins()
{
    Super::AllocateDefaultPins();

    // Input exec pin
    CreatePin(EGPD_Input,
              UEdGraphSchema_K2::PC_Exec,
              UEdGraphSchema_K2::PN_Execute);

    // Output exec pin
    CreatePin(EGPD_Output,
              UEdGraphSchema_K2::PC_Exec,
              UEdGraphSchema_K2::PN_Then);

    auto FilePin = CreatePin(EGPD_Input,
                             UEdGraphSchema_K2::PC_Object,
                             URiveFile::StaticClass(),
                             PN_File);
    FilePin->PinFriendlyName = FText::FromString(TEXT("Rive File Selection"));
    FilePin->PinToolTip =
        TEXT("File to create artboard from, connections are not allowed on "
             "this pin.\nSelect your desired file from the dropdown.");
    FilePin->bNotConnectable = true;

    auto OutputPin = CreatePin(EGPD_Output,
                               UEdGraphSchema_K2::PC_Object,
                               URiveArtboard::StaticClass(),
                               UEdGraphSchema_K2::PN_ReturnValue);
    OutputPin->PinToolTip = TEXT("Created Rive Artboard");
}

void UK2Node_MakeArtboard::PinDefaultValueChanged(UEdGraphPin* Pin)
{
    Super::PinDefaultValueChanged(Pin);
    UpdatePins(Pin);
}

void UK2Node_MakeArtboard::ReallocatePinsDuringReconstruction(
    TArray<UEdGraphPin*>& OldPins)
{
    Super::ReallocatePinsDuringReconstruction(OldPins);

    UEdGraphPin* OldFilePin = nullptr;
    UEdGraphPin* OldArtboardPin = nullptr;
    UEdGraphPin* OldAutoBindPin = nullptr;

    for (auto Pin : OldPins)
    {
        if (Pin->PinName == PN_File)
        {
            OldFilePin = Pin;
        }
        if (Pin->PinName == PN_ArtboardSource)
        {
            OldArtboardPin = Pin;
        }
        if (Pin->PinName == PN_AutoBind)
        {
            OldAutoBindPin = Pin;
        }
    }

    if (OldFilePin)
    {
        SelectedRiveFile = UPinTools::GetPinObjectValue<URiveFile>(OldFilePin);
        if (IsValid(SelectedRiveFile) && !SelectedRiveFile->GetHasData() &&
            !IsRunningCommandlet())
        {
            SelectedRiveFile->Initialize();
        }
    }
    if (OldArtboardPin)
    {
        auto Object = OldArtboardPin->PinType.PinSubCategoryObject.Get();
        auto NewPin = CreatePin(EGPD_Input,
                                UEdGraphSchema_K2::PC_Byte,
                                Object,
                                PN_ArtboardSource);
        NewPin->CopyPersistentDataFromOldPin(*OldArtboardPin);
    }
    if (OldAutoBindPin)
    {
        auto NewPin = CreatePin(EGPD_Input,
                                UEdGraphSchema_K2::PC_Boolean,
                                nullptr,
                                PN_AutoBind);
        NewPin->CopyPersistentDataFromOldPin(*OldAutoBindPin);
    }
}

void UK2Node_MakeArtboard::ExpandNode(FKismetCompilerContext& CompilerContext,
                                      UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);

    if (CheckForErrors(CompilerContext))
    {
        return;
    }

    auto Node_CallFunction =
        CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(
            this,
            SourceGraph);
    static const FName FunctionName =
        GET_FUNCTION_NAME_CHECKED(URiveFile, MakeArtboard);
    Node_CallFunction->FunctionReference.SetExternalMember(
        FunctionName,
        URiveFile::StaticClass());
    Node_CallFunction->AllocateDefaultPins();

    auto MyExecPin = GetExecPin();
    if (MyExecPin->HasAnyConnections())
    {
        auto NodeExecPin = Node_CallFunction->GetExecPin();
        UPinTools::MovePinLinksOrCopyDefaults(CompilerContext,
                                              MyExecPin,
                                              NodeExecPin);
    }

    auto MyThenPin = GetThenPin();
    if (MyThenPin->HasAnyConnections())
    {
        auto NodeThenPin = Node_CallFunction->GetThenPin();
        UPinTools::MovePinLinksOrCopyDefaults(CompilerContext,
                                              MyThenPin,
                                              NodeThenPin);
    }

    if (auto FilePin = GetFilePin())
    {
        auto SelfPin = Node_CallFunction->FindPin(TEXT("InRiveFile"));
        UPinTools::MovePinLinksOrCopyDefaults(CompilerContext,
                                              FilePin,
                                              SelfPin);
    }

    if (auto ArtboardPin = GetArtboardSourcePin())
    {
        auto TargetPin = Node_CallFunction->FindPin(TEXT("Name"));
        check(TargetPin);
        UPinTools::MovePinLinksOrCopyDefaults(CompilerContext,
                                              ArtboardPin,
                                              TargetPin);
    }

    if (auto AutoBindPin = GetAutoBindPin())
    {
        auto TargetPin =
            Node_CallFunction->FindPin(TEXT("inAutoBindViewModel"));
        check(TargetPin);
        UPinTools::MovePinLinksOrCopyDefaults(CompilerContext,
                                              AutoBindPin,
                                              TargetPin);
    }

    if (auto ReturnPin = GetReturnPin())
    {
        auto TargetPin = Node_CallFunction->GetReturnValuePin();
        CompilerContext.MovePinLinksToIntermediate(*ReturnPin, *TargetPin);
    }

    BreakAllNodeLinks();
}

bool UK2Node_MakeArtboard::CheckForErrors(
    FKismetCompilerContext& CompilerContext)
{
    if (!GetFilePin())
    {
        CompilerContext.MessageLog.Error(
            *FString::Printf(TEXT("Node %s requires a file selection."),
                             *GetName()),
            this);
        return true;
    }
    auto RiveFile = UPinTools::GetPinObjectValue<URiveFile>(GetFilePin());

    if (!RiveFile)
    {
        CompilerContext.MessageLog.Error(
            *FString::Printf(TEXT("Node %s requires a file."), *GetName()),
            this);
        return true;
    }

    if (!IsValid(RiveFile))
    {
        CompilerContext.MessageLog.Error(
            *FString::Printf(TEXT("Node %s selected rive file is invalid."),
                             *GetName()),
            this);
        return true;
    }

    // Unfortunately, on startup the file is not fully loaded, so we can't do
    // check properly in that situation.
    if (!RiveFile->GetHasData())
    {
        // Because of this ^ do a warning log instead of a compile error.
        UE_LOG(LogRiveNode, Warning, TEXT("Rive file is not loaded yet."));
        return false;
    }

    if (!GetArtboardSourcePin())
    {
        CompilerContext.MessageLog.Error(
            *FString::Printf(
                TEXT("Node %s requires a view model source selection."),
                *GetName()),
            this);
        return true;
    }

    return false;
}

void UK2Node_MakeArtboard::UpdatePins(UEdGraphPin* UpdatedPin)
{
    UEdGraphPin* InputFilePin = FindPin(PN_File);

    auto RiveFile = UPinTools::GetPinObjectValue<URiveFile>(InputFilePin);
    bool bNeedsEnumPins = RiveFile != nullptr;

    auto ArtboardPin = GetArtboardSourcePin();
    if ((bNeedsEnumPins && ArtboardPin == nullptr) &&
        SelectedRiveFile != RiveFile && IsValid(RiveFile))
    {
        SelectedRiveFile = RiveFile;
        UEnum* Enum = RiveFile->ArtboardEnum;
        auto Pin = CreatePin(EGPD_Input,
                             UEdGraphSchema_K2::PC_Byte,
                             Enum,
                             PN_ArtboardSource);
        Pin->PinFriendlyName = FText::FromString(TEXT("Artboard"));
        Pin->PinToolTip =
            TEXT("Select the desired artboard from the drop down.");
        Pin->bNotConnectable = true;

        auto AutoBindPin = CreatePin(EGPD_Input,
                                     UEdGraphSchema_K2::PC_Boolean,
                                     nullptr,
                                     PN_AutoBind);
        AutoBindPin->PinFriendlyName =
            FText::FromString(TEXT("Auto Bind Artboard"));
        AutoBindPin->PinToolTip =
            TEXT("If true, the artboard will be bound to the default view "
                 "model set in the editor. If you need to interact with the "
                 "view model directly, leave this false and create the view "
                 "model using \"MakeViewModel\" node instead. Then set the "
                 "view model with \"SetViewModel");
        AutoBindPin->bNotConnectable = false;

        auto Graph = GetGraph();
        Graph->NotifyNodeChanged(this);
    }
    else if (!bNeedsEnumPins && ArtboardPin)
    {
        RemovePin(ArtboardPin);
        RemovePin(GetAutoBindPin());
        SelectedRiveFile = nullptr;
    }

    GetGraph()->NotifyNodeChanged(this);
}
