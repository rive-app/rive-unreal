// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "Rive/RiveViewModelGetListIndexAsyncTask.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include <rive/command_server.hpp>
#include <rive/viewmodel/viewmodel_instance_symbol_list_index.hpp>
THIRD_PARTY_INCLUDES_END
#include "IRiveRendererModule.h"
#include "Async/Async.h"
#include "Logs/RiveLog.h"
#include "Rive/RiveViewModel.h"

URiveViewModelGetListIndexAsyncTask* URiveViewModelGetListIndexAsyncTask::
    RunGetListIndexAsyncTaskAsync(URiveViewModel* ViewModel)
{
    URiveViewModelGetListIndexAsyncTask* Action =
        NewObject<URiveViewModelGetListIndexAsyncTask>();
    Action->ViewModel = ViewModel;

    // Keeps the object alive until SetReadyToDestroy() is called
    Action->RegisterWithGameInstance(ViewModel);
    return Action;
}

void URiveViewModelGetListIndexAsyncTask::Activate()
{
    Super::Activate();

    if (!IsValid(ViewModel))
    {
        UE_LOG(
            LogRive,
            Error,
            TEXT("URiveViewModelGetListIndexAsyncTask ViewModel is invalid"));
        // We pass null here because the view model is invalid anyway.
        OnFailure.Broadcast(0, nullptr);
        SetReadyToDestroy();
        return;
    }

    auto Definition = ViewModel->GetViewModelDefinition();
    auto SymbolListIndexPropertyDef =
        Definition.PropertyDefinitions.FindByPredicate(
            [](const FRivePropertyData& P) {
                return P.Type == ERiveDataType::SymbolListIndex;
            });

    if (!SymbolListIndexPropertyDef)
    {
        UE_LOG(
            LogRive,
            Error,
            TEXT(
                "URiveViewModelGetListIndexAsyncTask No List Index Property Found on %s"),
            *ViewModel->GetName());
        OnFailure.Broadcast(0, ViewModel);
        SetReadyToDestroy();
        return;
    }

    auto NativeHandle = ViewModel->GetNativeHandle();
    if (NativeHandle == RIVE_NULL_HANDLE)
    {
        UE_LOG(
            LogRive,
            Error,
            TEXT("URiveViewModelGetListIndexAsyncTask NativeHandle is null"));
        OnFailure.Broadcast(0, ViewModel);
        SetReadyToDestroy();
        return;
    }

    auto& CommandBuilder = IRiveRendererModule::GetCommandBuilder();
    CommandBuilder.RunOnce([this,
                            NativeHandle,
                            Name = SymbolListIndexPropertyDef->Name](
                               rive::CommandServer* Server) {
        auto NativeInstance = Server->getViewModelInstance(NativeHandle);
        if (NativeInstance == RIVE_NULL_HANDLE)
        {
            UE_LOG(
                LogRive,
                Error,
                TEXT(
                    "URiveViewModelGetListIndexAsyncTask view model native handle is invalid"));
            AsyncTask(ENamedThreads::GameThread, [this]() {
                OnFailure.Broadcast(0, ViewModel);
                SetReadyToDestroy();
            });
            return;
        }

        int32 Index = -1;

        if (auto IndexProperty =
                NativeInstance->propertyListIndex(TCHAR_TO_UTF8(*Name)))
        {
            Index = IndexProperty->value();
        }
        else
        {
            UE_LOG(
                LogRive,
                Error,
                TEXT(
                    "URiveViewModelGetListIndexAsyncTask failed to find list index property named %s"),
                *Name);
        }

        AsyncTask(ENamedThreads::GameThread, [this, Index]() {
            // Broadcast fires the execution pin and passes data to BP
            if (Index != -1)
            {
                OnSuccess.Broadcast(Index, ViewModel);
            }
            else
            {
                OnFailure.Broadcast(0, ViewModel);
            }

            // Clean up the object for garbage collection
            SetReadyToDestroy();
        });
    });
}