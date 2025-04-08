#include "Rive/ViewModel/RiveViewModelInstanceValue.h"
#include "Rive/ViewModel/RiveViewModelInstance.h"

void URiveViewModelInstanceValue::Initialize(
    rive::ViewModelInstanceValueRuntime* InViewModelInstanceValue)
{
    ViewModelInstanceValuePtr = InViewModelInstanceValue;
}

void URiveViewModelInstanceValue::BeginDestroy()
{
    ClearCallbacks();
    Super::BeginDestroy();
}

void URiveViewModelInstanceValue::HandleCallbacks()
{
    if (!ViewModelInstanceValuePtr)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("HandleCallbacks() ViewModelInstanceValuePtr is null."));

        return;
    }

    if (ViewModelInstanceValuePtr->hasChanged())
    {
        ViewModelInstanceValuePtr->clearChanges();
        OnValueChangedMulti.Broadcast();
    }
}

void URiveViewModelInstanceValue::ClearCallbacks()
{
    OnValueChangedMulti.Clear();
    if (OnRemoveCallbackProperty.IsBound())
    {
        OnRemoveCallbackProperty.Execute(this);
    }
}

void URiveViewModelInstanceValue::BindToValueChange(
    FOnValueChangedDelegate Delegate)
{
    if (!Delegate.IsBound())
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("BindToValueChange failed: Delegate is not bound."));

        return;
    }

    if (ViewModelInstanceValuePtr && ViewModelInstanceValuePtr->hasChanged())
    {
        ViewModelInstanceValuePtr->clearChanges();
    }

    OnValueChangedMulti.AddUnique(Delegate);

    if (OnAddCallbackProperty.IsBound())
    {
        OnAddCallbackProperty.Execute(this);
    }
}

void URiveViewModelInstanceValue::UnbindFromValueChange(
    FOnValueChangedDelegate Delegate)
{
    if (!Delegate.IsBound())
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("UnbindFromValueChange failed: Delegate is not bound."));

        return;
    }

    OnValueChangedMulti.Remove(Delegate);

    if (OnRemoveCallbackProperty.IsBound())
    {
        OnRemoveCallbackProperty.Execute(this);
    }
}

void URiveViewModelInstanceValue::UnbindAllFromValueChange()
{
    ClearCallbacks();
}
