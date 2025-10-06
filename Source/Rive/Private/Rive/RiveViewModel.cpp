// Fill out your copyright notice in the Description page of Project Settings.

#include "Rive/RiveViewModel.h"

#include "IRiveRendererModule.h"
#include "RiveRenderer.h"
#include "Logs/RiveLog.h"
#include "rive/command_queue.hpp"
#include "rive/command_server.hpp"
#include "Rive/RiveArtboard.h"
#include "Rive/RiveFile.h"
#include "Rive/RiveUtils.h"
#include "Engine/BlueprintGeneratedClass.h"

TMap<rive::ViewModelInstanceHandle, URiveViewModel*>
    URiveViewModel::ViewModelInstances;

static rive::DataType RiveDataTypeToDataType(ERiveDataType Type)
{
    switch (Type)
    {
        case ERiveDataType::None:
            return rive::DataType::none;
        case ERiveDataType::String:
            return rive::DataType::string;
        case ERiveDataType::Number:
            return rive::DataType::number;
        case ERiveDataType::Boolean:
            return rive::DataType::boolean;
        case ERiveDataType::Color:
            return rive::DataType::color;
        case ERiveDataType::List:
            return rive::DataType::list;
        case ERiveDataType::EnumType:
            return rive::DataType::enumType;
        case ERiveDataType::Trigger:
            return rive::DataType::trigger;
        case ERiveDataType::ViewModel:
            return rive::DataType::viewModel;
        case ERiveDataType::AssetImage:
            return rive::DataType::assetImage;
        case ERiveDataType::Artboard:
            return rive::DataType::artboard;
    }
    return rive::DataType::none;
}

class FRiveViewModelListener final
    : public rive::CommandQueue::ViewModelInstanceListener
{
public:
    FRiveViewModelListener(TWeakObjectPtr<URiveViewModel> ListeningViewModel) :
        ListeningViewModel(ListeningViewModel)
    {}

    virtual void onViewModelInstanceError(
        const rive::ViewModelInstanceHandle Handle,
        uint64_t RequestId,
        std::string Error) override
    {
        FUTF8ToTCHAR Conversion(Error.c_str());
        UE_LOG(LogRive,
               Error,
               TEXT("Rive View Model RequestId: %llu Error {%s}"),
               RequestId,
               Conversion.Get());
    }

    virtual void onViewModelDeleted(const rive::ViewModelInstanceHandle Handle,
                                    uint64_t RequestId) override
    {
        check(IsInGameThread());
        if (auto StrongViewModel = ListeningViewModel.Pin();
            StrongViewModel.IsValid())
        {
            UE_LOG(LogRive,
                   Display,
                   TEXT("Rive View Model Named %s deleted"),
                   *StrongViewModel->GetName());
        }
        else
        {
            UE_LOG(LogRive,
                   Display,
                   TEXT("Rive View Model Handle %p deleted"),
                   Handle);
        }

        delete this;
    }

    virtual void onViewModelDataReceived(
        const rive::ViewModelInstanceHandle Handle,
        uint64_t RequestId,
        rive::CommandQueue::ViewModelInstanceData Data) override
    {
        if (auto StrongViewModel = ListeningViewModel.Pin())
        {
            StrongViewModel->OnViewModelDataReceived(RequestId, MoveTemp(Data));
        }
    }

    virtual void onViewModelListSizeReceived(
        const rive::ViewModelInstanceHandle Handle,
        uint64_t RequestId,
        std::string Path,
        size_t Size) override
    {
        if (auto StrongViewModel = ListeningViewModel.Pin())
        {
            StrongViewModel->OnViewModelListSizeReceived(MoveTemp(Path), Size);
        }
    }

private:
    TWeakObjectPtr<URiveViewModel> ListeningViewModel = nullptr;
};

UClass* URiveViewModel::LoadGeneratedClassForViewModel(
    const UObject* Context,
    const URiveFile* File,
    const FString& ViewModelName)
{
    FString SanatizedViewModelName =
        RiveUtils::SanitizeObjectName(ViewModelName);
    FString ClassName = SanatizedViewModelName + TEXT(".") +
                        SanatizedViewModelName + TEXT("_C");
    static FString FolderPath = RiveUtils::GetPackagePathForFile(File);
    FString FullPath = FolderPath + TEXT("/") + ClassName;
    return LoadClass<URiveViewModel>(Context->GetOutermost(), *FullPath);
}

constexpr bool GetIsPropertyTypeWithDefault(ERiveDataType Type)
{
    switch (Type)
    {
        case ERiveDataType::String:
        case ERiveDataType::Number:
        case ERiveDataType::Boolean:
        case ERiveDataType::Color:
        case ERiveDataType::EnumType:
        case ERiveDataType::ViewModel:
        case ERiveDataType::Artboard:
            return true;
        case ERiveDataType::AssetImage:
        case ERiveDataType::List:
        case ERiveDataType::Trigger:
        case ERiveDataType::None:
            break;
    }

    return false;
}

void URiveViewModel::Initialize(
    FRiveCommandBuilder& Builder,
    const URiveFile* OwningFile,
    const FViewModelDefinition& InViewModelDefinition,
    const FString& InstanceName)
{
    check(OwningFile);
    check(!InViewModelDefinition.Name.IsEmpty());
    ViewModelDefinition = InViewModelDefinition;

    if (InstanceName == GViewModelInstanceBlankName)
    {
        NativeViewModelInstance =
            Builder.CreateBlankViewModel(OwningFile->GetNativeFileHandle(),
                                         FName(ViewModelDefinition.Name));
    }
    else if (InstanceName == GViewModelInstanceDefaultName ||
             InstanceName.IsEmpty())
    {
        NativeViewModelInstance =
            Builder.CreateDefaultViewModel(OwningFile->GetNativeFileHandle(),
                                           FName(ViewModelDefinition.Name),
                                           new FRiveViewModelListener(this));
    }
    else
    {
        NativeViewModelInstance =
            Builder.CreateViewModel(OwningFile->GetNativeFileHandle(),
                                    FName(ViewModelDefinition.Name),
                                    FName(InstanceName),
                                    new FRiveViewModelListener(this));
    }

    const FString ResolvedInstanceName =
        InstanceName == GViewModelInstanceDefaultName
            ? ViewModelDefinition.DefaultInstanceName
            : InstanceName;
    // There is no reason to auto-subscribe if we aren't a generated view model.
    if (bIsGenerated)
    {
        int32 DefaultValueIndex = 0;
        const auto ViewModelDefault =
            ViewModelDefinition.InstanceDefaults.FindByPredicate(
                [ResolvedInstanceName](const auto& ViewModelDefault) {
                    return ViewModelDefault.InstanceName ==
                           ResolvedInstanceName;
                });
        UBlueprintGeneratedClass* GeneratedClass =
            Cast<UBlueprintGeneratedClass>(GetClass());
        check(ViewModelDefault);
        check(GeneratedClass);
        for (int32 Index = 0;
             Index < ViewModelDefinition.PropertyDefinitions.Num();
             ++Index)
        {
            const auto& PropertyDefinition =
                ViewModelDefinition.PropertyDefinitions[Index];

            if (ViewModelDefault &&
                GetIsPropertyTypeWithDefault(PropertyDefinition.Type))
            {
                const FPropertyDefaultData& DefaultData =
                    ViewModelDefault->PropertyValues[DefaultValueIndex++];
                check(DefaultData.Name == PropertyDefinition.Name);
                const FString& DefaultValue = DefaultData.Value;
                switch (PropertyDefinition.Type)
                {
                    case ERiveDataType::String:
                    {
                        if (FStrProperty* Prop = FindFProperty<FStrProperty>(
                                GeneratedClass,
                                *PropertyDefinition.Name))
                        {
                            Prop->SetPropertyValue_InContainer(this,
                                                               DefaultValue);
                        }
                    }
                    break;
                    case ERiveDataType::Number:
                    {
                        if (FFloatProperty* Prop =
                                FindFProperty<FFloatProperty>(
                                    GeneratedClass,
                                    *PropertyDefinition.Name))
                        {
                            Prop->SetPropertyValue_InContainer(
                                this,
                                FCString::Atof(*DefaultValue));
                        }
                    }
                    break;
                    case ERiveDataType::Boolean:
                    {
                        if (FBoolProperty* Prop = FindFProperty<FBoolProperty>(
                                GeneratedClass,
                                *PropertyDefinition.Name))
                        {
                            Prop->SetPropertyValue_InContainer(
                                this,
                                DefaultValue == TEXT("True") ? true : false);
                        }
                    }
                    break;
                    case ERiveDataType::Color:
                    {
                        if (FStructProperty* Prop =
                                FindFProperty<FStructProperty>(
                                    GeneratedClass,
                                    *PropertyDefinition.Name))
                        {
                            FLinearColor* ColorValue =
                                Prop->ContainerPtrToValuePtr<FLinearColor>(
                                    this);
                            rive::ColorInt ColorInt =
                                FCString::Atoi(*DefaultValue);
                            float Values[4];
                            rive::UnpackColorToRGBA32F(ColorInt, Values);
                            ColorValue->R = Values[0];
                            ColorValue->G = Values[1];
                            ColorValue->B = Values[2];
                            ColorValue->A = Values[3];
                        }
                    }
                    break;
                    case ERiveDataType::EnumType:
                    {
                        if (FByteProperty* Prop = FindFProperty<FByteProperty>(
                                GeneratedClass,
                                *PropertyDefinition.Name))
                        {
                            UEnum* Enum = Prop->GetIntPropertyEnum();
                            check(Enum);
                            Prop->SetPropertyValue_InContainer(
                                this,
                                Enum->GetIndexByNameString(DefaultValue));
                        }
                    }
                    break;
                    case ERiveDataType::ViewModel:
                    {
                        if (FObjectProperty* Prop =
                                FindFProperty<FObjectProperty>(
                                    GeneratedClass,
                                    *PropertyDefinition.Name))
                        {
                            FString ViewModelType;
                            FString ViewModelInstanceName;
                            ensure(DefaultValue.Split(TEXT("/"),
                                                      &ViewModelType,
                                                      &ViewModelInstanceName));
                            TObjectPtr<URiveViewModel> NestedViewModel =
                                URiveFile::CreateViewModelByName(
                                    OwningFile,
                                    ViewModelType,
                                    ViewModelInstanceName);
                            Prop->SetPropertyValue_InContainer(this,
                                                               NestedViewModel);
                            Builder.SetViewModelViewModel(
                                NativeViewModelInstance,
                                PropertyDefinition.Name,
                                NestedViewModel->NativeViewModelInstance);
                        }
                    }
                    break;
                    case ERiveDataType::Artboard:
                    {
                        // Do nothing for this but it is a valid default. Maybe
                        // we will do something with it later.
                    }
                    break;
                    case ERiveDataType::AssetImage:
                    case ERiveDataType::List:
                    default:
                        continue;
                }
            }

            // View model properties do not have subscriptions
            if (PropertyDefinition.Type == ERiveDataType::ViewModel)
            {
                continue;
            }

            Builder.SubscribeToProperty(
                NativeViewModelInstance,
                PropertyDefinition.Name,
                RiveDataTypeToDataType(PropertyDefinition.Type));

            if (PropertyDefinition.Type == ERiveDataType::Trigger)
            {
                FScriptDelegate GenericDelegate;
                const FString TriggerName =
                    GET_FUNCTION_NAME_STRING_CHECKED(URiveTriggerDelegate,
                                                     SetTrigger);
                URiveTriggerDelegate* TriggerDelegate =
                    NewObject<URiveTriggerDelegate>(
                        this,
                        FName(TriggerName + TEXT("_TriggerDelegate")));
                TriggerDelegate->OwningViewModel = this;
                TriggerDelegate->TriggerIndex = Index;
                GenericDelegate.BindUFunction(TriggerDelegate,
                                              FName(TriggerName));
                if (auto DelegateProperty =
                        FindFProperty<FMulticastDelegateProperty>(
                            GetClass(),
                            *PropertyDefinition.Name))
                {
                    DelegateProperty->AddDelegate(GenericDelegate, this);
                }
            }
        }
    }
    // This handle should not exist in the map.
    check(!ViewModelInstances.Contains(NativeViewModelInstance));
    ViewModelInstances.Add(NativeViewModelInstance, this);
}

bool URiveViewModel::GetBoolValue(const FString& PropertyName,
                                  bool& OutValue) const
{
    if (auto BoolProperty = FindFProperty<FBoolProperty>(*PropertyName))
    {
        OutValue = BoolProperty->GetPropertyValue_InContainer(this);
        return true;
    }

    return false;
}

bool URiveViewModel::GetColorValue(const FString& PropertyName,
                                   FLinearColor& OutColor) const
{
    if (auto BoolProperty = FindFProperty<FStructProperty>(*PropertyName))
    {
        if (const auto LinearColor =
                BoolProperty->ContainerPtrToValuePtr<FLinearColor>(this))
        {
            OutColor = *LinearColor;
            return true;
        }
    }
    return false;
}

bool URiveViewModel::GetStringValue(const FString& PropertyName,
                                    FString& OutString) const
{
    if (auto StrProperty = FindFProperty<FStrProperty>(*PropertyName))
    {
        OutString = StrProperty->GetPropertyValue_InContainer(this);
        return true;
    }
    return false;
}

bool URiveViewModel::GetEnumValue(const FString& PropertyName,
                                  FString& EnumValue) const
{
    if (auto EnumProperty = FindFProperty<FByteProperty>(*PropertyName))
    {
        const uint64 EnumIndex =
            EnumProperty->GetUnsignedIntPropertyValue_InContainer(this);
        const FName NameValue =
            EnumProperty->GetIntPropertyEnum()->GetNameByIndex(EnumIndex);
        EnumValue = NameValue.ToString();
        return true;
    }
    return false;
}

bool URiveViewModel::GetNumberValue(const FString& PropertyName,
                                    float& OutNumber) const
{
    if (auto FloatProperty = FindFProperty<FFloatProperty>(*PropertyName))
    {
        OutNumber = FloatProperty->GetPropertyValue_InContainer(this);
        return true;
    }
    return false;
}

bool URiveViewModel::GetListSize(const FString& PropertyName,
                                 size_t& OutSize) const
{
    if (auto ListProperty = FindFProperty<FStructProperty>(*PropertyName))
    {
        if (const auto List =
                ListProperty->ContainerPtrToValuePtr<FRiveList>(this))
        {
            OutSize = List->ListSize;
            return true;
        }
    }
    return false;
}

void URiveViewModel::K2_AddFieldValueChangedDelegate(
    FFieldNotificationId InFieldId,
    FFieldValueChangedDynamicDelegate InDelegate)
{
    if (InFieldId.IsValid())
    {
        const UE::FieldNotification::FFieldId FieldId =
            GetFieldNotificationDescriptor().GetField(GetClass(),
                                                      InFieldId.FieldName);
        ensureMsgf(FieldId.IsValid(),
                   TEXT("The field should be compiled correctly."));
        Delegates.Add(this, FieldId, InDelegate);
    }
}

void URiveViewModel::K2_RemoveFieldValueChangedDelegate(
    FFieldNotificationId InFieldId,
    FFieldValueChangedDynamicDelegate InDelegate)
{
    RemoveFieldValueChangedDelegate(InFieldId, MoveTemp(InDelegate));
}

void URiveViewModel::K2_AppendToList(FRiveList List, URiveViewModel* Value)
{
    auto RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    check(RiveRenderer);
    auto& Builder = RiveRenderer->GetCommandBuilder();
    Builder.AppendViewModelList(NativeViewModelInstance,
                                List.Path,
                                Value->NativeViewModelInstance);
    Builder.RunOnce([InstanceHandle = Value->NativeViewModelInstance](
                        rive::CommandServer* CommandServer) {
        auto instance = CommandServer->getViewModelInstance(InstanceHandle);
        auto name = instance->name();
        UE_LOG(LogRive,
               Display,
               TEXT("Rive View Model List Append %hs"),
               name.c_str());
    });
}

void URiveViewModel::K2_InsertToList(FRiveList List,
                                     int32 Index,
                                     URiveViewModel* Value)
{
    auto RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    check(RiveRenderer);
    auto& Builder = RiveRenderer->GetCommandBuilder();
    Builder.InsertViewModelList(NativeViewModelInstance,
                                List.Path,
                                Value->NativeViewModelInstance,
                                Index);
}

void URiveViewModel::K2_RemoveFromList(FRiveList List, int32 Index)
{
    auto RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    check(RiveRenderer);
    auto& Builder = RiveRenderer->GetCommandBuilder();
    Builder.RemoveViewModelList(NativeViewModelInstance, List.Path, Index);
}

void URiveViewModel::SetTrigger(const FString& TriggerName)
{
    if (bIsInDataCallback)
        return;
    auto RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    check(RiveRenderer);
    auto& Builder = RiveRenderer->GetCommandBuilder();
    uint64_t Id =
        Builder.SetViewModelTrigger(NativeViewModelInstance, TriggerName);
    UE_LOG(LogRive,
           Verbose,
           TEXT("Set Trigger Request with name %s Request Id %llu"),
           *TriggerName,
           Id);
}

void URiveViewModel::OnViewModelDataReceived(
    uint64_t RequestId,
    rive::CommandQueue::ViewModelInstanceData Data)
{
    FUTF8ToTCHAR Conversion(Data.metaData.name.c_str());
    FProperty* Property =
        FindFProperty<FProperty>(GetClass(), Conversion.Get());
    if (!Property)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to find property \"%s\" for view model named "
                    "\"%s\" with class \"%s\" on sub update !"),
               Conversion.Get(),
               *GetName(),
               *GetClass()->GetName());
        return;
    }

    bIsInDataCallback = true;
    switch (Data.metaData.type)
    {
        case rive::DataType::string:
            if (auto StrProperty = CastField<FStrProperty>(Property))
            {
                FUTF8ToTCHAR StringValueConversion(Data.stringValue.c_str());
                StrProperty->SetPropertyValue_InContainer(
                    this,
                    StringValueConversion.Get());
            }
            break;
        case rive::DataType::boolean:
            if (auto BoolProperty = CastField<FBoolProperty>(Property))
            {
                BoolProperty->SetPropertyValue_InContainer(this,
                                                           Data.boolValue);
            }
            break;
        case rive::DataType::color:
            if (auto StructProperty = CastField<FStructProperty>(Property))
            {
                auto ColorValue =
                    StructProperty->ContainerPtrToValuePtr<FLinearColor>(this);
                float colors[4];
                rive::UnpackColorToRGBA32F(Data.colorValue, colors);
                ColorValue->R = colors[0];
                ColorValue->G = colors[1];
                ColorValue->B = colors[2];
                ColorValue->A = colors[3];
            }
            break;
        case rive::DataType::enumType:
            if (auto ByteProperty = CastField<FByteProperty>(Property))
            {
                UEnum* EnumValue = ByteProperty->GetIntPropertyEnum();
                FUTF8ToTCHAR EnumStr(Data.stringValue.c_str());
                FName FullEnumName(EnumStr.Get());
                auto NewIndex = EnumValue->GetIndexByName(FullEnumName);
                ByteProperty->SetPropertyValue_InContainer(this, NewIndex);
            }
            break;
        case rive::DataType::trigger:
            if (auto DelegateProperty =
                    CastField<FMulticastDelegateProperty>(Property))
            {
                auto Delegate = DelegateProperty->GetMulticastDelegate(
                    DelegateProperty->ContainerPtrToValuePtr<void>(this));
                check(Delegate);
                // We don't have any inputs or outputs for triggers
                Delegate->ProcessMulticastDelegate<UObject>(nullptr);
            }
            break;
        case rive::DataType::viewModel:
        case rive::DataType::artboard:
        case rive::DataType::assetImage:
            // What to do here ?
            if (auto ObjectProperty = CastField<FObjectProperty>(Property))
            {
                // ObjectProperty->SetObjectPropertyValue_InContainer(this, );
            }
            break;
        case rive::DataType::number:
            if (auto FloatProperty = CastField<FFloatProperty>(Property))
            {
                auto ValuePtr =
                    FloatProperty->ContainerPtrToValuePtr<float>(this);
                *ValuePtr = Data.numberValue;
            }
            break;
        case rive::DataType::list:
            // now we request the new list size since we know it changed
            {
                auto& Builder = IRiveRendererModule::GetCommandBuilder();
                Builder.GetPropertyListSize(NativeViewModelInstance,
                                            Property->GetName());
                break;
            }
        case rive::DataType::none:
        case rive::DataType::integer:
        case rive::DataType::symbolListIndex:
            RIVE_UNREACHABLE();
    }

    if (UBlueprintGeneratedClass* BlueprintClass =
            Cast<UBlueprintGeneratedClass>(GetClass()))
    {
        for (int32 Index = 0; Index < BlueprintClass->FieldNotifies.Num();
             ++Index)
        {
            UE::FieldNotification::FFieldId FieldId(
                BlueprintClass->FieldNotifies[Index].GetFieldName(),
                Index + BlueprintClass->FieldNotifiesStartBitNumber);
            if (FieldId.GetName() == Property->GetName())
            {
                Delegates.Broadcast(this, FieldId);
                break;
            }
        }
    }

    bIsInDataCallback = false;
}

void URiveViewModel::OnViewModelListSizeReceived(std::string Path,
                                                 size_t ListSize)
{
    FUTF8ToTCHAR Conversion(Path.c_str());
    FStructProperty* Property =
        FindFProperty<FStructProperty>(Conversion.Get());
    if (!Property)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to find list property to update !"));
        return;
    }

    FRiveList* List = Property->ContainerPtrToValuePtr<FRiveList>(this);
    check(List);
    List->ListSize = static_cast<int32>(ListSize);

    if (UBlueprintGeneratedClass* BlueprintClass =
            Cast<UBlueprintGeneratedClass>(GetClass()))
    {
        for (int32 Index = 0; Index < BlueprintClass->FieldNotifies.Num();
             ++Index)
        {
            UE::FieldNotification::FFieldId FieldId(
                BlueprintClass->FieldNotifies[Index].GetFieldName(),
                Index + BlueprintClass->FieldNotifiesStartBitNumber);
            if (FieldId.GetName() == Property->GetName())
            {
                Delegates.Broadcast(this, FieldId);
                break;
            }
        }
    }
}

void URiveViewModel::BeginDestroy()
{
    if (NativeViewModelInstance != RIVE_NULL_HANDLE)
    {
        auto RiveRenderer = IRiveRendererModule::Get().GetRenderer();
        check(RiveRenderer);
        auto& Builder = RiveRenderer->GetCommandBuilder();
        for (auto PropertyDefinition : ViewModelDefinition.PropertyDefinitions)
        {
            Builder.SubscribeToProperty(
                NativeViewModelInstance,
                PropertyDefinition.Name,
                RiveDataTypeToDataType(PropertyDefinition.Type));
        }
        Builder.DestroyViewModel(NativeViewModelInstance);
        check(ViewModelInstances.Contains(NativeViewModelInstance));
        ViewModelInstances.Remove(NativeViewModelInstance);
    }
    UObject::BeginDestroy();
}

void URiveViewModel::OnUpdatedField(UE::FieldNotification::FFieldId InFieldId)
{
    auto Varient = InFieldId.ToVariant(this);
    if (!ensure(Varient.IsValid()))
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveViewModel::OnUpdatedField Invalid FieldId %s."),
               *InFieldId.GetName().GetPlainNameString());
        return;
    }
    auto Property = Varient.GetProperty();
    if (!ensure(Property))
    {
        UE_LOG(
            LogRive,
            Error,
            TEXT("URiveViewModel::OnUpdatedField Field %s is not a property."),
            *InFieldId.GetName().GetPlainNameString());
        return;
    }

    if (auto StrongArtboard = OwningArtboard.Pin(); StrongArtboard.IsValid())
    {
        StrongArtboard->UnsettleStateMachine();
    }

    auto PropName = Property->GetName();
    auto RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    check(RiveRenderer);
    auto& Builder = RiveRenderer->GetCommandBuilder();
    if (auto StrProperty = CastField<FStrProperty>(Property))
    {
        const FString StringValue =
            StrProperty->GetPropertyValue_InContainer(this);
        Builder.SetViewModelString(NativeViewModelInstance,
                                   PropName,
                                   StringValue);
    }
    else if (auto ByteProperty = CastField<FByteProperty>(Property))
    {
        UEnum* EnumValue = ByteProperty->GetIntPropertyEnum();
        const uint64 EnumIndex =
            ByteProperty->GetUnsignedIntPropertyValue_InContainer(this);
        const FName NameValue = EnumValue->GetNameByIndex(EnumIndex);
        FString StringValue = NameValue.ToString();
        FString Left, Right;
        if (StringValue.Split("::", &Left, &Right))
        {
            StringValue = Right;
        }
        Builder.SetViewModelEnum(NativeViewModelInstance,
                                 PropName,
                                 StringValue);
    }
    else if (auto BoolProperty = CastField<FBoolProperty>(Property))
    {
        const bool BoolValue = BoolProperty->GetPropertyValue_InContainer(this);
        Builder.SetViewModelBool(NativeViewModelInstance, PropName, BoolValue);
    }
    else if (auto FloatProperty = CastField<FFloatProperty>(Property))
    {
        const float FloatValue =
            FloatProperty->GetPropertyValue_InContainer(this);
        Builder.SetViewModelNumber(NativeViewModelInstance,
                                   PropName,
                                   FloatValue);
    }
    else if (auto StructProperty = CastField<FStructProperty>(Property))
    {
        if (StructProperty->Struct->GetFName() == NAME_LinearColor)
        {
            const FLinearColor* StructValue =
                StructProperty->ContainerPtrToValuePtr<FLinearColor>(this);
            Builder.SetViewModelColor(NativeViewModelInstance,
                                      PropName,
                                      *StructValue);
        }
    }
    else if (auto ObjectProperty = CastField<FObjectProperty>(Property))
    {
        auto ObjectClass = ObjectProperty->PropertyClass;
        auto Object = ObjectProperty->GetObjectPropertyValue_InContainer(this);

        if (ObjectClass == URiveViewModel::StaticClass())
        {
            if (URiveViewModel* ViewModelObject = Cast<URiveViewModel>(Object))
            {
                // Var was set.
                Builder.SetViewModelViewModel(
                    NativeViewModelInstance,
                    PropName,
                    ViewModelObject->NativeViewModelInstance);
            }
        }
        else if (ObjectClass == UTexture2D::StaticClass())
        {
            if (UTexture2D* TextureObject = Cast<UTexture2D>(Object))
            {
                // Var was set.
                Builder.SetViewModelImage(NativeViewModelInstance,
                                          PropName,
                                          TextureObject);
            }
        }
        else if (ObjectClass == URiveArtboard::StaticClass())
        {
            if (URiveArtboard* ArtboardObject = Cast<URiveArtboard>(Object))
            {
                // Var was set.
                Builder.SetViewModelArtboard(
                    NativeViewModelInstance,
                    PropName,
                    ArtboardObject->GetNativeArtboardHandle());
            }
        }
        else
        {
            UE_LOG(LogRive,
                   Error,
                   TEXT("URiveViewModel::OnUpdatedField Unsupported Object "
                        "Type %s."),
                   *ObjectClass->GetName());
        }
    }
}

bool URiveViewModel::RemoveFieldValueChangedDelegate(
    FFieldNotificationId InFieldId,
    const FFieldValueChangedDynamicDelegate& InDelegate)
{
    auto RemoveResult = Delegates.Remove(InDelegate);
    return RemoveResult.bRemoved;
}

void URiveViewModel::FFieldNotificationClassDescriptor::ForEachField(
    const UClass* Class,
    TFunctionRef<bool(::UE::FieldNotification::FFieldId FieldId)> Callback)
    const
{
    if (const UBlueprintGeneratedClass* BPClass =
            Cast<const UBlueprintGeneratedClass>(Class))
    {
        BPClass->ForEachFieldNotify(Callback, true);
    }
}
