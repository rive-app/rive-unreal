// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "INotifyFieldValueChanged.h"
#include "UObject/Object.h"
#include "RiveCommandBuilder.h"
#include "FieldNotificationDelegate.h"
#include "RiveInternalTypes.h"
#include "RiveViewModel.generated.h"

class URiveViewModel;
class URiveArtboard;
struct FViewModelDefinition;
class URiveFile;

// Struct that represents a list property.
USTRUCT(BlueprintType)
struct FRiveList
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    FString Path;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    int32 ListSize = 0;

    // This is not an exhaustive list of view models. It is only to prevent GC
    // of added view models.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    TArray<TObjectPtr<URiveViewModel>> ViewModels;
};

class URiveTriggerDelegate;

/**
 *
 */
UCLASS(Blueprintable, BlueprintType)
class RIVE_API URiveViewModel : public UObject, public INotifyFieldValueChanged
{
    GENERATED_BODY()
public:
    void Initialize(FRiveCommandBuilder&,
                    const URiveFile* OwningFile,
                    const FViewModelDefinition& InViewModelDefinition,
                    const FString& InstanceName);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    bool bIsGenerated = false;

    // Getters for generated view model properties
    // These all use blueprint reflection to get the values. This means you must
    // check HasDefaultValues() before using these or the value returned may be
    // incorrect.

    UFUNCTION(BlueprintPure, Category = "Rive|Data Binding")
    bool GetBoolValue(const FString& PropertyName, bool& OutValue) const;
    UFUNCTION(BlueprintPure, Category = "Rive|Data Binding")
    bool GetColorValue(const FString& PropertyName,
                       FLinearColor& OutColor) const;
    UFUNCTION(BlueprintPure, Category = "Rive|Data Binding")
    bool GetStringValue(const FString& PropertyName, FString& OutString) const;
    UFUNCTION(BlueprintPure, Category = "Rive|Data Binding")
    bool GetEnumValue(const FString& PropertyName, FString& EnumValue) const;
    UFUNCTION(BlueprintPure, Category = "Rive|Data Binding")
    bool GetNumberValue(const FString& PropertyName, float& OutNumber) const;
    UFUNCTION(BlueprintPure, Category = "Rive|Data Binding")
    bool GetListSize(const FString& PropertyName, int32& OutSize) const;
    UFUNCTION(BlueprintPure, Category = "Rive|Data Binding")
    bool ContainsListsByName(const FString& ListName) const;
    UFUNCTION(BlueprintPure, Category = "Rive|Data Binding")
    // This is a copy that you get. So do not expect to be able to modify the
    // list value here.
    bool GetListByName(const FString ListName, FRiveList& OutList) const;
    UFUNCTION(BlueprintPure, Category = "Rive|Data Binding")
    bool ContainsLists(FRiveList InList) const;
    UFUNCTION(BlueprintCallable, Category = "Rive|Data Binding")
    bool SetBoolValue(const FString& PropertyName, bool InValue);
    UFUNCTION(BlueprintCallable, Category = "Rive|Data Binding")
    bool SetColorValue(const FString& PropertyName, FLinearColor InColor);
    UFUNCTION(BlueprintCallable, Category = "Rive|Data Binding")
    bool SetStringValue(const FString& PropertyName, const FString& InString);
    UFUNCTION(BlueprintCallable, Category = "Rive|Data Binding")
    bool SetEnumValue(const FString& PropertyName, const FString& InEnumValue);
    UFUNCTION(BlueprintCallable, Category = "Rive|Data Binding")
    bool SetNumberValue(const FString& PropertyName, float InNumber);
    UFUNCTION(BlueprintCallable, Category = "Rive|Data Binding")
    bool SetImageValue(const FString& PropertyName, UTexture* InImage);
    UFUNCTION(BlueprintCallable, Category = "Rive|Data Binding")
    bool SetArtboardValue(const FString& PropertyName,
                          URiveArtboard* InArtboard);
    UFUNCTION(BlueprintCallable, Category = "Rive|Data Binding")
    bool SetViewModelValue(const FString& PropertyName,
                           URiveViewModel* InViewModel);
    UFUNCTION(BlueprintCallable, Category = "Rive|Data Binding")
    bool AppendToList(const FString& ListName, URiveViewModel* Value);
    UFUNCTION(BlueprintCallable, Category = "Rive|Data Binding")
    bool InsertToList(const FString& ListName,
                      int32 Index,
                      URiveViewModel* Value);

    UFUNCTION(BlueprintCallable, Category = "Rive|Data Binding")
    bool RemoveFromList(const FString& ListName, URiveViewModel* Value);

    UFUNCTION(BlueprintCallable, Category = "Rive|Data Binding")
    bool RemoveFromListAtIndex(const FString& ListName, int32 Index);

    UFUNCTION(BlueprintCallable,
              Category = "FieldNotify",
              meta = (DisplayName = "Add Field Value Changed Delegate",
                      ScriptName = "AddFieldValueChangedDelegate"))
    void K2_AddFieldValueChangedDelegate(
        FFieldNotificationId FieldId,
        FFieldValueChangedDynamicDelegate Delegate);

    UFUNCTION(BlueprintCallable,
              Category = "FieldNotify",
              meta = (DisplayName = "Remove Field Value Changed Delegate",
                      ScriptName = "RemoveFieldValueChangedDelegate"))
    void K2_RemoveFieldValueChangedDelegate(
        FFieldNotificationId FieldId,
        FFieldValueChangedDynamicDelegate Delegate);

    UFUNCTION(BlueprintCallable,
              Category = "Rive|Data Binding|Lists",
              meta = (DisplayName = "Append View Model At End Of List",
                      ScriptName = "AppendToList"))
    void K2_AppendToList(FRiveList List, URiveViewModel* Value);

    UFUNCTION(BlueprintCallable,
              Category = "Rive|Data Binding|Lists",
              meta = (DisplayName = "Insert View Model Into List",
                      ScriptName = "InsertToList"))
    void K2_InsertToList(FRiveList List, int32 Index, URiveViewModel* Value);

    UFUNCTION(BlueprintCallable,
              Category = "Rive|Data Binding|Lists",
              meta = (DisplayName = "Remove View Model From List",
                      ScriptName = "RemoveFromList"))
    void K2_RemoveFromList(FRiveList List, URiveViewModel* Value);

    UFUNCTION(BlueprintCallable,
              Category = "Rive|Data Binding|Lists",
              meta = (DisplayName = "Remove View Model From List At Index",
                      ScriptName = "RemoveFromListAtIndex"))
    void K2_RemoveFromListAtIndex(FRiveList List, int32 Index);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModel")
    void SetTrigger(const FString& TriggerName);

    // INotifyFieldValueChanged Implementation
    /** Add a delegate that will be notified when the FieldId is value changed.
     */
    virtual FDelegateHandle AddFieldValueChangedDelegate(
        UE::FieldNotification::FFieldId InFieldId,
        FFieldValueChangedDelegate InNewDelegate) override
    {
        return (InFieldId.IsValid())
                   ? Delegates.Add(this, InFieldId, MoveTemp(InNewDelegate))
                   : FDelegateHandle();
    }

    /** Remove a delegate that was added. */
    virtual bool RemoveFieldValueChangedDelegate(
        UE::FieldNotification::FFieldId InFieldId,
        FDelegateHandle InHandle) override
    {
        if (InFieldId.IsValid() && InHandle.IsValid())
        {
            return Delegates.RemoveFrom(this, InFieldId, InHandle).bRemoved;
        }
        return false;
    }

    /** Remove all the delegate that are bound to the specified UserObject. */
    virtual int32 RemoveAllFieldValueChangedDelegates(
        FDelegateUserObjectConst InUserObject) override
    {
        if (InUserObject)
        {
            return Delegates.RemoveAll(this, InUserObject).RemoveCount;
        }
        return false;
    }

    /** Remove all the delegate that are bound to the specified Field and
     * UserObject. */
    virtual int32 RemoveAllFieldValueChangedDelegates(
        UE::FieldNotification::FFieldId InFieldId,
        FDelegateUserObjectConst InUserObject) override
    {
        if (InFieldId.IsValid() && InUserObject)
        {
            return Delegates.RemoveAll(this, InFieldId, InUserObject)
                .RemoveCount;
        }
        return false;
    }

    /** @returns the list of all the field that can notify when their value
     * changes. */
    virtual const UE::FieldNotification::IClassDescriptor&
    GetFieldNotificationDescriptor() const
    {
        static FFieldNotificationClassDescriptor Local;
        return Local;
    }

    /** Broadcast to the registered delegate that the FieldId value changed. */
    virtual void BroadcastFieldValueChanged(
        UE::FieldNotification::FFieldId InFieldId) override
    {
        OnUpdatedField(InFieldId);
        if (InFieldId.IsValid())
        {
            Delegates.Broadcast(this, InFieldId);
        }
    }

    struct FFieldNotificationClassDescriptor
        : public UE::FieldNotification::IClassDescriptor
    {
        virtual void ForEachField(
            const UClass* Class,
            TFunctionRef<bool(UE::FieldNotification::FFieldId FielId)> Callback)
            const override;
    };

    rive::ViewModelInstanceHandle GetNativeHandle() const
    {
        return NativeViewModelInstance;
    }
    // Get a view model instance from a native handle.
    // Should only be called from the game thread.
    static URiveViewModel* GetViewModelInstance(
        rive::ViewModelInstanceHandle Handle)
    {
        check(IsInGameThread());
        URiveViewModel** InstancePtr = ViewModelInstances.Find(Handle);
        return InstancePtr != nullptr ? *InstancePtr : nullptr;
    }

    void OnViewModelDataReceived(
        uint64_t RequestId,
        rive::CommandQueue::ViewModelInstanceData Data);
    void OnViewModelListSizeReceived(std::string Path, size_t ListSize);

    FORCEINLINE void SetOwningArtboard(TWeakObjectPtr<URiveArtboard> Artboard)
    {
        OwningArtboard = MoveTemp(Artboard);
    }

    FORCEINLINE void SetViewModelDefinition(
        const FViewModelDefinition& InViewModelDefinition)
    {
        ViewModelDefinition = InViewModelDefinition;
    }

    const FViewModelDefinition& GetViewModelDefinition() const
    {
        return ViewModelDefinition;
    }

protected:
    virtual void BeginDestroy() override;

    void OnUpdatedField(UE::FieldNotification::FFieldId InFieldId);

    // This is used to unsettle the state machine when a property is updated.
    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Rive)
    TWeakObjectPtr<URiveArtboard> OwningArtboard;

    // Set when the blueprint is generated
    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Rive)
    FViewModelDefinition ViewModelDefinition;

private:
    bool RemoveFieldValueChangedDelegate(
        FFieldNotificationId FieldId,
        const FFieldValueChangedDynamicDelegate& Delegate);

    void UpdateListWithViewModelData(const FString& ListPath,
                                     URiveViewModel* Value,
                                     bool bRemove);

    rive::ViewModelInstanceHandle NativeViewModelInstance = RIVE_NULL_HANDLE;

    // Map of every rive handle to view model instance. This is used to lookup
    // an existing view model instance for liststs or other callbacks that use
    // Native Handles.
    static TMap<rive::ViewModelInstanceHandle, URiveViewModel*>
        ViewModelInstances;

    UE::FieldNotification::FFieldMulticastDelegate Delegates;

    // Used for forwarding trigger names to the delegate callback without having
    // to make changes to how multicast delegates work
    TArray<URiveTriggerDelegate> TriggerDelegates;

    // Used for triggers so that we don't accidentally call the delegate twice.
    TArray<FName> IgnoredTriggerCallbacks;

    void UnsettleStateMachine(const TCHAR* Context) const;

    // Checked in SetTrigger to make sure we don't infinite recurse when a
    // trigger is fired from the riv state machine
    bool bIsInDataCallback = false;

    friend class URiveTriggerDelegate;
};

UCLASS(Blueprintable, BlueprintType)
class URiveTriggerDelegate : public UObject
{
    GENERATED_BODY()
public:
    int32 TriggerIndex = -1;

    TWeakObjectPtr<URiveViewModel> OwningViewModel;

    UFUNCTION(BlueprintCallable, Category = "Rive | Triggers")
    void SetTrigger();
};
