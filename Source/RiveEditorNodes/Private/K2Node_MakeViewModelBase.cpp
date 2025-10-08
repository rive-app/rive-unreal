// Fill out your copyright notice in the Description page of Project Settings.

#include "K2Node_MakeViewModelBase.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "KismetCompiler.h"
#include "Logs/LogRiveNode.h"
#include "Rive/RiveFile.h"
#include "Rive/RiveViewModel.h"

static void MovePinLinksOrCopyDefaults(FKismetCompilerContext& CompilerContext,
                                       UEdGraphPin* Source,
                                       UEdGraphPin* Dest)
{
    if (Source->LinkedTo.Num() > 0)
    {
        CompilerContext.MovePinLinksToIntermediate(*Source, *Dest);
    }
    else
    {
        Dest->DefaultObject = Source->DefaultObject;
        Dest->DefaultValue = Source->DefaultValue;
        Dest->DefaultTextValue = Source->DefaultTextValue;
    }
}

static FString GetPinStringValue(UEdGraphPin* Pin)
{
    check(Pin);
    if (Pin->HasAnyConnections())
    {
        for (auto ConnectedPin : Pin->LinkedTo)
        {
            if (ConnectedPin->DefaultObject)
                return ConnectedPin->GetDefaultAsString();
        }
    }

    return Pin->GetDefaultAsString();
}

template <typename ReturnType>
static TObjectPtr<ReturnType> GetPinObjectValue(UEdGraphPin* Pin)
{
    check(Pin);
    if (Pin->HasAnyConnections())
    {
        for (auto ConnectedPin : Pin->LinkedTo)
        {
            if (ConnectedPin->DefaultObject)
                return Cast<ReturnType>(ConnectedPin->DefaultObject);
        }
    }

    return Cast<ReturnType>(Pin->DefaultObject);
}

FName UK2Node_MakeViewModelBase::PN_File = TEXT("FileInput");
FName UK2Node_MakeViewModelBase::PN_ViewModelSource =
    TEXT("ViewModelSourceInput");
FName UK2Node_MakeViewModelBase::PN_ViewModelInstance =
    TEXT("ViewModelInstanceInput");

FText UK2Node_MakeViewModelBase::GetMenuCategory() const
{
    return FText::FromString("Rive");
}

void UK2Node_MakeViewModelBase::BeginDestroy()
{
    if (IsValid(SelectedRiveFile))
    {
        SelectedRiveFile->OnDataReady.RemoveAll(this);
    }
    Super::BeginDestroy();
}

int UK2Node_MakeViewModelBase::GetIndexFromSelection(const FString& Selection)
{
    return INDEX_NONE;
}

void UK2Node_MakeViewModelBase::GetMenuActions(
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

UEdGraphPin* UK2Node_MakeViewModelBase::GetReturnPin() const
{
    UEdGraphPin* Pin = FindPin(UEdGraphSchema_K2::PN_ReturnValue);
    check(Pin != nullptr && Pin->Direction == EGPD_Output);
    return Pin;
}

void UK2Node_MakeViewModelBase::AllocateDefaultPins()
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
        TEXT("File to create view model from, connections are not allowed on "
             "this pin.\nSelect your desired file from the dropdown.");
    FilePin->bNotConnectable = true;

    auto OutputPin = CreatePin(EGPD_Output,
                               UEdGraphSchema_K2::PC_Object,
                               URiveViewModel::StaticClass(),
                               UEdGraphSchema_K2::PN_ReturnValue);
    OutputPin->PinToolTip = TEXT("Created View Model");
    OutputPin->bHidden = true;
}

void UK2Node_MakeViewModelBase::PinDefaultValueChanged(UEdGraphPin* Pin)
{
    Super::PinDefaultValueChanged(Pin);
    UpdatePins(Pin);
    if (Pin == GetViewModelSourcePin())
    {
        check(SelectedRiveFile);
        check(SelectedIndex != INDEX_NONE);

        auto SelectedViewModel = GetViewModelDefinitionFromIndex(SelectedIndex);
        check(SelectedViewModel);
        ViewModelClass = SelectedRiveFile->GetGeneratedClassForViewModel(
            *SelectedViewModel->Name);
        // Make sure we have the correct "type" of class as a bunch of
        // intermediates get created
        if (ensure(ViewModelClass))
        {
            ViewModelClass = ViewModelClass->GetAuthoritativeClass();
        }
        else
        {
            UE_LOG(LogRiveNode,
                   Warning,
                   TEXT("View model class not loaded, using default instead."));
            ViewModelClass = URiveViewModel::StaticClass();
        }

        UEdGraphPin* RetPin = GetReturnPin();
        check(RetPin);
        if (RetPin->PinType.PinSubCategoryObject != ViewModelClass)
        {
            RetPin->PinType.PinSubCategoryObject = ViewModelClass;
            RetPin->PinToolTip = FString::Format(TEXT("Created {0} View Model"),
                                                 {ViewModelClass->GetName()});
            RetPin->bHidden = false;
            GetGraph()->NotifyNodeChanged(this);
        }
    }
}

void UK2Node_MakeViewModelBase::ReallocatePinsDuringReconstruction(
    TArray<UEdGraphPin*>& OldPins)
{
    Super::ReallocatePinsDuringReconstruction(OldPins);

    UEdGraphPin* OldFilePin = nullptr;
    UEdGraphPin* OldViewModelSourcePin = nullptr;
    UEdGraphPin* OldViewModelInstancePin = nullptr;
    UEdGraphPin* OldReturnPin = nullptr;

    for (auto Pin : OldPins)
    {
        if (Pin->PinName == PN_File)
        {
            OldFilePin = Pin;
        }
        if (Pin->PinName == PN_ViewModelSource)
        {
            OldViewModelSourcePin = Pin;
        }
        if (Pin->PinName == PN_ViewModelInstance)
        {
            OldViewModelInstancePin = Pin;
        }
        if (Pin->PinName == UEdGraphSchema_K2::PN_ReturnValue)
        {
            OldReturnPin = Pin;
        }
    }

    if (OldFilePin)
    {
        SelectedRiveFile = GetPinObjectValue<URiveFile>(OldFilePin);
        if (IsValid(SelectedRiveFile) && !SelectedRiveFile->GetHasData() &&
            !IsRunningCommandlet())
        {
            SelectedRiveFile->Initialize();
        }
    }
    if (OldViewModelSourcePin)
    {
        auto Object = OldViewModelSourcePin->PinType.PinSubCategoryObject.Get();
        auto NewPin = CreatePin(EGPD_Input,
                                UEdGraphSchema_K2::PC_Byte,
                                Object,
                                PN_ViewModelSource);
        NewPin->CopyPersistentDataFromOldPin(*OldViewModelSourcePin);
    }
    if (OldViewModelInstancePin)
    {
        auto Object =
            OldViewModelInstancePin->PinType.PinSubCategoryObject.Get();
        auto NewPin = CreatePin(EGPD_Input,
                                UEdGraphSchema_K2::PC_Byte,
                                Object,
                                PN_ViewModelInstance);
        NewPin->CopyPersistentDataFromOldPin(*OldViewModelInstancePin);
    }
    if (OldReturnPin && !OldReturnPin->bHidden)
    {
        ViewModelClass =
            Cast<UClass>(OldReturnPin->PinType.PinSubCategoryObject.Get());
        if (ViewModelClass)
        {
            ViewModelClass = ViewModelClass->GetAuthoritativeClass();
        }
        if (!ViewModelClass)
        {
            ViewModelClass = URiveViewModel::StaticClass();
        }
        UEdGraphPin* RetPin = GetReturnPin();
        RetPin->PinType.PinSubCategoryObject = ViewModelClass;
        RetPin->PinToolTip = FString::Format(TEXT("Created {0} View Model"),
                                             {ViewModelClass->GetName()});
        RetPin->bHidden = false;
    }

    if (SelectedRiveFile && OldViewModelSourcePin)
    {
        if (SelectedRiveFile->GetHasData())
        {
            SelectedIndex =
                GetIndexFromSelection(GetPinStringValue(OldViewModelSourcePin));
        }
        else
        {
            const auto Selection = GetPinStringValue(OldViewModelSourcePin);
            auto handle = SelectedRiveFile->OnDataReady.AddLambda(
                [this, Selection](URiveFile*) {
                    SelectedRiveFile->OnDataReady.RemoveAll(this);
                    SelectedIndex = GetIndexFromSelection(Selection);
                });
        }
    }
}

void UK2Node_MakeViewModelBase::ExpandNode(
    FKismetCompilerContext& CompilerContext,
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
    SetFunctionOnIntermediateNode(Node_CallFunction);
    Node_CallFunction->AllocateDefaultPins();

    auto MyExecPin = GetExecPin();
    if (MyExecPin->HasAnyConnections())
    {
        auto NodeExecPin = Node_CallFunction->GetExecPin();
        MovePinLinksOrCopyDefaults(CompilerContext, MyExecPin, NodeExecPin);
    }

    auto MyThenPin = GetThenPin();
    if (MyThenPin->HasAnyConnections())
    {
        auto NodeThenPin = Node_CallFunction->GetThenPin();
        MovePinLinksOrCopyDefaults(CompilerContext, MyThenPin, NodeThenPin);
    }

    if (auto FilePin = GetFilePin())
    {
        auto TargetPin = Node_CallFunction->FindPin(TEXT("InputFile"));
        MovePinLinksOrCopyDefaults(CompilerContext, FilePin, TargetPin);
    }

    if (auto ViewModelPin = GetViewModelSourcePin())
    {
        auto TargetPin = GetTargetPinForViewModelSource(Node_CallFunction);
        check(TargetPin);
        MovePinLinksOrCopyDefaults(CompilerContext, ViewModelPin, TargetPin);
    }

    if (auto ViewModelInstacePin = GetViewModelInstancePin())
    {
        auto TargetPin = Node_CallFunction->FindPin(TEXT("InstanceName"));
        MovePinLinksOrCopyDefaults(CompilerContext,
                                   ViewModelInstacePin,
                                   TargetPin);
    }

    if (auto ReturnPin = GetReturnPin())
    {
        auto TargetPin = Node_CallFunction->GetReturnValuePin();
        // Taken from K2Node_GenericConstructObject.cpp:134 in case this ever
        // changes or needs to.
        if (TargetPin && ReturnPin)
        {
            TargetPin->PinType = ReturnPin->PinType;
        }
        CompilerContext.MovePinLinksToIntermediate(*ReturnPin, *TargetPin);
    }

    BreakAllNodeLinks();
}

bool UK2Node_MakeViewModelBase::CheckForErrors(
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
    auto RiveFile = GetPinObjectValue<URiveFile>(GetFilePin());

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

    if (!GetViewModelSourcePin())
    {
        CompilerContext.MessageLog.Error(
            *FString::Printf(
                TEXT("Node %s requires a view model source selection."),
                *GetName()),
            this);
        return true;
    }

    if (!GetViewModelInstancePin())
    {
        CompilerContext.MessageLog.Error(
            *FString::Printf(
                TEXT("Node %s requires a view model instance selection."),
                *GetName()),
            this);
        return true;
    }

    return false;
}

void UK2Node_MakeViewModelBase::UpdatePins(UEdGraphPin* UpdatedPin)
{
    UEdGraphPin* InputFilePin = FindPin(PN_File);

    SelectedIndex = INDEX_NONE;

    auto RiveFile = GetPinObjectValue<URiveFile>(InputFilePin);
    bool bNeedsEnumPins = RiveFile != nullptr;

    if ((bNeedsEnumPins && GetViewModelSourcePin() == nullptr) ||
        (SelectedRiveFile != RiveFile && IsValid(RiveFile)))
    {
        SelectedRiveFile = RiveFile;
        GenerateViewModelSourcePin(RiveFile);
    }
    else if (!bNeedsEnumPins && GetViewModelSourcePin())
    {
        if (auto VPin = GetViewModelSourcePin())
            RemovePin(VPin);
        if (auto VPin = GetViewModelInstancePin())
            RemovePin(VPin);
    }

    auto ViewModelPin = GetViewModelSourcePin();
    auto ViewModelInstancePin = GetViewModelInstancePin();

    if (ViewModelPin &&
        (!ViewModelInstancePin || ViewModelInstancePin != UpdatedPin))
    {
        check(SelectedRiveFile != nullptr);
        auto Value = GetPinStringValue(ViewModelPin);
        SelectedIndex = GetIndexFromSelection(Value);
        if (SelectedIndex != INDEX_NONE)
        {
            auto ViewModelDefinition =
                GetViewModelDefinitionFromIndex(SelectedIndex);
            if (ensure(ViewModelDefinition != nullptr))
                GenerateViewModelInstanceEnumPin(*ViewModelDefinition);
        }
    }

    GetGraph()->NotifyNodeChanged(this);
}

void UK2Node_MakeViewModelBase::GenerateViewModelSourcePin(
    TObjectPtr<URiveFile> RiveFile)
{
    // We never want more than one View Model Pin, So always remove if we have
    // one.
    if (auto Pin = GetViewModelSourcePin())
    {
        // Remove pin if we need to update it
        RemovePin(Pin);
    }

    if (auto Pin = GetViewModelInstancePin())
    {
        RemovePin(Pin);
    }

    auto Lamda = [this](TObjectPtr<URiveFile> RiveFile) {
        UEnum* Enum = GenerateViewModelSourceEnum(RiveFile);

        auto Pin = CreatePin(EGPD_Input,
                             UEdGraphSchema_K2::PC_Byte,
                             Enum,
                             PN_ViewModelSource);
        Pin->PinFriendlyName = GetViewModelInputName();
        Pin->PinToolTip =
            TEXT("Select the desired view model from the drop down.\nThis "
                 "represents the type of View Model to make.\nThis pin does "
                 "not allow connections.");
        Pin->bNotConnectable = true;

        auto Graph = GetGraph();
        Graph->NotifyNodeChanged(this);
    };

    if (RiveFile->GetHasData())
    {
        Lamda(RiveFile);
    }
    else
    {
        RiveFile->OnDataReady.AddLambda(Lamda);
    }
}

void UK2Node_MakeViewModelBase::GenerateViewModelInstanceEnumPin(
    const FViewModelDefinition& SelectedViewModel)
{
    // If the user was able to select a view model it means the data is already
    // loaded, and we don't need to check again here. We never want more then
    // one view model instance pin, so always remove if we have one.
    if (auto OldPin = GetViewModelInstancePin())
    {
        // Remove pin if we need to update it
        RemovePin(OldPin);
    }

    auto Enum = SelectedRiveFile->GetViewModelInstanceEnum(SelectedViewModel);
    check(Enum);
    auto Pin = CreatePin(EGPD_Input,
                         UEdGraphSchema_K2::PC_Byte,
                         Enum,
                         PN_ViewModelInstance);
    Pin->PinFriendlyName = GetViewModelInstanceInputName();
    Pin->PinToolTip =
        TEXT("Select the desired view model instance from the drop down.\nThis "
             "only affects the default values of this View Model.\nThis pin "
             "does not allow connections.");
    Pin->bNotConnectable = true;
    if (SelectedViewModel.InstanceNames.Num() &&
        !SelectedViewModel.InstanceNames[0].IsEmpty())
    {
        Pin->DefaultValue = SelectedViewModel.InstanceNames[0];
    }
    else
    {
        Pin->DefaultValue = GViewModelInstanceDefaultName.ToString();
    }

    GetReturnPin()->bHidden = false;

    auto Graph = GetGraph();
    Graph->NotifyNodeChanged(this);
}
