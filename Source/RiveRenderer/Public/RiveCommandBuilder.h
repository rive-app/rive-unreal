#pragma once
#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
namespace rive
{
class Renderer;
}

THIRD_PARTY_INCLUDES_START
#include "rive/animation/state_machine_instance.hpp"
#include "rive/command_queue.hpp"
THIRD_PARTY_INCLUDES_END

#include "RiveTypes.h"
#include "RiveCommandBuilder.generated.h"

class FRiveRenderTarget;

// Nothing here needs to be garbage collected, so we don't need to make it a
// USTRUCT.
struct FDrawArtboardCommand
{
    rive::ArtboardHandle Handle;
    FBox2f AlignmentBox;
    ERiveAlignment Alignment;
    ERiveFitType FitType;
    float ScaleFactor;
};

typedef TFunction<void(rive::CommandServer*)> ServerSideCallback;
typedef TFunction<
    void(rive::DrawKey, rive::CommandServer*, rive::Renderer*, rive::Factory*)>
    DirectDrawCallback;

// Contains all commands for a given render target, all commands held here are
// expected to happen between BeginFrame and Flush.
USTRUCT()
struct FRiveCommandSet
{
    GENERATED_BODY();

    // The unique key for drawing to this render target
    rive::DrawKey DrawKey = RIVE_NULL_HANDLE;

    // Array of commands to submit specifically for drawing Artboards.
    TArray<FDrawArtboardCommand> ArtboardDrawCommands;
    // Array of draw commands to submit that could be anything.
    TArray<DirectDrawCallback> DrawCommands;
};

/*
 * FRiveCommandBuilder must be created and used in the GameThread. It manages
 * the CommandQueue for sending commands to the command server.
 */
struct RIVERENDERER_API FRiveCommandBuilder
{
    FRiveCommandBuilder() {}
    FRiveCommandBuilder(rive::rcp<rive::CommandQueue> CommandQueue);

    FRiveCommandBuilder(FRiveCommandBuilder&) = delete;
    void operator=(const FRiveCommandBuilder&) const = delete;

    void Reset()
    {
        Commands.Empty();
        DrawCommands.Empty();
    }

    rive::FileHandle LoadFile(
        std::vector<uint8_t> FileData,
        rive::CommandQueue::FileListener* FileListener = nullptr,
        uint64_t* outRequestId = nullptr)
    {
        if (outRequestId)
        {
            *outRequestId = ++CurrentRequestId;
            return CommandQueue->loadFile(MoveTemp(FileData),
                                          FileListener,
                                          *outRequestId);
        }
        return CommandQueue->loadFile(MoveTemp(FileData), FileListener);
    }

    uint64_t DestroyFile(rive::FileHandle FileHandle)
    {
        CommandQueue->deleteFile(FileHandle, ++CurrentRequestId);
        return CurrentRequestId;
    }

    rive::ArtboardHandle CreateDefaultArtboard(
        rive::FileHandle File,
        rive::CommandQueue::ArtboardListener* Listener = nullptr,
        uint64_t* outRequestId = nullptr)
    {
        if (outRequestId)
        {
            *outRequestId = ++CurrentRequestId;
            return CommandQueue->instantiateDefaultArtboard(File,
                                                            Listener,
                                                            *outRequestId);
        }
        return CommandQueue->instantiateDefaultArtboard(File, Listener);
    }

    rive::ArtboardHandle CreateArtboard(
        rive::FileHandle File,
        const FString& ArtboardName,
        rive::CommandQueue::ArtboardListener* Listener = nullptr,
        uint64_t* outRequestId = nullptr)
    {
        FTCHARToUTF8 Convert(*ArtboardName);
        if (outRequestId)
        {
            *outRequestId = ++CurrentRequestId;
            return CommandQueue->instantiateArtboardNamed(File,
                                                          Convert.Get(),
                                                          Listener,
                                                          *outRequestId);
        }
        return CommandQueue->instantiateArtboardNamed(File,
                                                      Convert.Get(),
                                                      Listener);
    }

    rive::ArtboardHandle CreateArtboard(
        rive::FileHandle File,
        const FName& ArtboardName,
        rive::CommandQueue::ArtboardListener* Listener = nullptr,
        uint64_t* outRequestId = nullptr)
    {
        FTCHARToUTF8 Convert(*ArtboardName.ToString());
        if (outRequestId)
        {
            *outRequestId = ++CurrentRequestId;
            return CommandQueue->instantiateArtboardNamed(File,
                                                          Convert.Get(),
                                                          Listener,
                                                          *outRequestId);
        }
        return CommandQueue->instantiateArtboardNamed(File,
                                                      Convert.Get(),
                                                      Listener);
    }

    rive::ViewModelInstanceHandle CreateViewModel(
        rive::FileHandle File,
        const FName& ViewModelName,
        const FName& ViewModelInstanceName,
        rive::CommandQueue::ViewModelInstanceListener* Listener = nullptr,
        uint64_t* outRequestId = nullptr)
    {
        FTCHARToUTF8 ConvertViewModel(*ViewModelName.ToString());
        FTCHARToUTF8 ConvertInstance(*ViewModelInstanceName.ToString());
        if (outRequestId)
        {
            *outRequestId = ++CurrentRequestId;
            return CommandQueue->instantiateViewModelInstanceNamed(
                File,
                ConvertViewModel.Get(),
                ConvertInstance.Get(),
                Listener,
                *outRequestId);
        }
        return CommandQueue->instantiateViewModelInstanceNamed(
            File,
            ConvertViewModel.Get(),
            ConvertInstance.Get(),
            Listener);
    }

    rive::ViewModelInstanceHandle CreateDefaultViewModel(
        rive::FileHandle File,
        const FName& ViewModelName,
        rive::CommandQueue::ViewModelInstanceListener* Listener = nullptr,
        uint64_t* outRequestId = nullptr)
    {
        FTCHARToUTF8 ConvertViewModel(*ViewModelName.ToString());
        if (outRequestId)
        {
            *outRequestId = ++CurrentRequestId;
            return CommandQueue->instantiateDefaultViewModelInstance(
                File,
                ConvertViewModel.Get(),
                Listener,
                *outRequestId);
        }
        return CommandQueue->instantiateDefaultViewModelInstance(
            File,
            ConvertViewModel.Get(),
            Listener);
    }

    rive::ViewModelInstanceHandle CreateDefaultViewModelForArtboard(
        rive::FileHandle File,
        rive::ArtboardHandle Artboard,
        rive::CommandQueue::ViewModelInstanceListener* Listener = nullptr,
        uint64_t* outRequestId = nullptr)
    {
        if (outRequestId)
        {
            *outRequestId = ++CurrentRequestId;
            return CommandQueue->instantiateDefaultViewModelInstance(
                File,
                Artboard,
                Listener,
                *outRequestId);
        }
        return CommandQueue->instantiateDefaultViewModelInstance(File,
                                                                 Artboard,
                                                                 Listener);
    }

    rive::ViewModelInstanceHandle CreateBlankViewModel(
        rive::FileHandle File,
        const FName& ViewModelName,
        rive::CommandQueue::ViewModelInstanceListener* Listener = nullptr,
        uint64_t* outRequestId = nullptr)
    {
        FTCHARToUTF8 ConvertViewModel(*ViewModelName.ToString());
        if (outRequestId)
        {
            *outRequestId = ++CurrentRequestId;
            return CommandQueue->instantiateBlankViewModelInstance(
                File,
                ConvertViewModel.Get(),
                Listener,
                *outRequestId);
        }
        return CommandQueue->instantiateBlankViewModelInstance(
            File,
            ConvertViewModel.Get(),
            Listener);
    }

    uint64_t GetPropertyValue(rive::ViewModelInstanceHandle ViewModel,
                              const FString& Name,
                              rive::DataType Type)
    {
        FTCHARToUTF8 ConvertName(*Name);
        switch (Type)
        {
            case rive::DataType::boolean:
                CommandQueue->requestViewModelInstanceBool(ViewModel,
                                                           ConvertName.Get(),
                                                           ++CurrentRequestId);
                break;
            case rive::DataType::string:
                CommandQueue->requestViewModelInstanceString(
                    ViewModel,
                    ConvertName.Get(),
                    ++CurrentRequestId);
                break;
            case rive::DataType::enumType:
                CommandQueue->requestViewModelInstanceEnum(ViewModel,
                                                           ConvertName.Get(),
                                                           ++CurrentRequestId);
                break;
            case rive::DataType::number:
                CommandQueue->requestViewModelInstanceNumber(
                    ViewModel,
                    ConvertName.Get(),
                    ++CurrentRequestId);
                break;
            case rive::DataType::color:
                CommandQueue->requestViewModelInstanceColor(ViewModel,
                                                            ConvertName.Get(),
                                                            ++CurrentRequestId);
                break;
            case rive::DataType::none:
            case rive::DataType::list:
            case rive::DataType::trigger:
            case rive::DataType::viewModel:
            case rive::DataType::integer:
            case rive::DataType::symbolListIndex:
            case rive::DataType::assetImage:
            case rive::DataType::artboard:
                return -1;
        }
        return CurrentRequestId;
    }

    uint64_t GetPropertyListSize(rive::ViewModelInstanceHandle ViewModel,
                                 const FString& Name)
    {
        FTCHARToUTF8 ConvertName(*Name);
        CommandQueue->requestViewModelInstanceListSize(ViewModel,
                                                       ConvertName.Get(),
                                                       ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t SubscribeToProperty(rive::ViewModelInstanceHandle ViewModel,
                                 const FString& Name,
                                 rive::DataType Type)
    {
        FTCHARToUTF8 ConvertName(*Name);
        CommandQueue->subscribeToViewModelProperty(ViewModel,
                                                   ConvertName.Get(),
                                                   Type,
                                                   ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t UnsubscribeFromProperty(rive::ViewModelInstanceHandle ViewModel,
                                     const FString& Name,
                                     rive::DataType Type)
    {
        FTCHARToUTF8 ConvertName(*Name);
        CommandQueue->unsubscribeToViewModelProperty(ViewModel,
                                                     ConvertName.Get(),
                                                     Type,
                                                     ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t SetViewModelString(rive::ViewModelInstanceHandle ViewModel,
                                const FString& Name,
                                const FString& Value)
    {
        FTCHARToUTF8 ConvertName(*Name);
        FTCHARToUTF8 ConvertValue(*Value);
        CommandQueue->setViewModelInstanceString(ViewModel,
                                                 ConvertName.Get(),
                                                 ConvertValue.Get(),
                                                 ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t SetViewModelNumber(rive::ViewModelInstanceHandle ViewModel,
                                const FString& Name,
                                float Value)
    {
        FTCHARToUTF8 ConvertName(*Name);
        CommandQueue->setViewModelInstanceNumber(ViewModel,
                                                 ConvertName.Get(),
                                                 Value,
                                                 ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t SetViewModelBool(rive::ViewModelInstanceHandle ViewModel,
                              const FString& Name,
                              bool Value)
    {
        FTCHARToUTF8 ConvertName(*Name);
        CommandQueue->setViewModelInstanceBool(ViewModel,
                                               ConvertName.Get(),
                                               Value,
                                               ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t SetViewModelTrigger(rive::ViewModelInstanceHandle ViewModel,
                                 const FString& Name)
    {
        FTCHARToUTF8 ConvertName(*Name);
        CommandQueue->fireViewModelTrigger(ViewModel,
                                           ConvertName.Get(),
                                           ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t SetViewModelColor(rive::ViewModelInstanceHandle ViewModel,
                               const FString& Name,
                               FLinearColor Value)
    {
        FTCHARToUTF8 ConvertName(*Name);
        rive::ColorInt Color = rive::colorARGB(Value.A * 255,
                                               Value.R * 255,
                                               Value.G * 255,
                                               Value.B * 255);
        CommandQueue->setViewModelInstanceColor(ViewModel,
                                                ConvertName.Get(),
                                                Color,
                                                ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t SetViewModelEnum(rive::ViewModelInstanceHandle ViewModel,
                              const FString& Name,
                              const FString& Value)
    {
        FTCHARToUTF8 ConvertName(*Name);
        FTCHARToUTF8 ConvertValue(*Value);
        CommandQueue->setViewModelInstanceEnum(ViewModel,
                                               ConvertName.Get(),
                                               ConvertValue.Get(),
                                               ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t SetViewModelImage(rive::ViewModelInstanceHandle ViewModel,
                               const FString& Name,
                               rive::RenderImageHandle Value)
    {
        FTCHARToUTF8 ConvertName(*Name);
        CommandQueue->setViewModelInstanceImage(ViewModel,
                                                ConvertName.Get(),
                                                Value,
                                                ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t SetViewModelImage(rive::ViewModelInstanceHandle ViewModel,
                               const FString& Name,
                               UTexture2D* Value);

    uint64_t SetViewModelViewModel(rive::ViewModelInstanceHandle ViewModel,
                                   const FString& Name,
                                   rive::ViewModelInstanceHandle Value)
    {
        FTCHARToUTF8 ConvertName(*Name);
        CommandQueue->setViewModelInstanceNestedViewModel(ViewModel,
                                                          ConvertName.Get(),
                                                          Value,
                                                          ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t SetViewModelArtboard(rive::ViewModelInstanceHandle ViewModel,
                                  const FString& Name,
                                  rive::ArtboardHandle Value)
    {
        FTCHARToUTF8 ConvertName(*Name);
        CommandQueue->setViewModelInstanceArtboard(ViewModel,
                                                   ConvertName.Get(),
                                                   Value,
                                                   ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t AppendViewModelList(rive::ViewModelInstanceHandle ViewModel,
                                 const FString& Path,
                                 rive::ViewModelInstanceHandle ToAppend)
    {
        FTCHARToUTF8 ConvertPath(*Path);
        CommandQueue->appendViewModelInstanceListViewModel(ViewModel,
                                                           ConvertPath.Get(),
                                                           ToAppend,
                                                           ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t InsertViewModelList(rive::ViewModelInstanceHandle ViewModel,
                                 const FString& Path,
                                 rive::ViewModelInstanceHandle ToInsert,
                                 int32_t Index)
    {
        FTCHARToUTF8 ConvertPath(*Path);
        CommandQueue->insertViewModelInstanceListViewModel(ViewModel,
                                                           ConvertPath.Get(),
                                                           ToInsert,
                                                           Index,
                                                           ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t RemoveViewModelList(rive::ViewModelInstanceHandle ViewModel,
                                 const FString& Path,
                                 int32_t Index)
    {
        FTCHARToUTF8 ConvertPath(*Path);
        CommandQueue->removeViewModelInstanceListViewModel(ViewModel,
                                                           ConvertPath.Get(),
                                                           Index,
                                                           nullptr,
                                                           ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t DestroyArtboard(rive::ArtboardHandle Artboard)
    {
        CommandQueue->deleteArtboard(Artboard, ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t DestroyViewModel(rive::ViewModelInstanceHandle ViewModel)
    {
        CommandQueue->deleteViewModelInstance(ViewModel, ++CurrentRequestId);
        return CurrentRequestId;
    }

    rive::StateMachineHandle CreateDefaultStateMachine(
        rive::ArtboardHandle Artboard,
        rive::CommandQueue::StateMachineListener* Listener = nullptr,
        uint64_t* outRequestId = nullptr)
    {
        if (outRequestId)
        {
            *outRequestId = ++CurrentRequestId;
            return CommandQueue->instantiateDefaultStateMachine(Artboard,
                                                                Listener,
                                                                *outRequestId);
        }
        return CommandQueue->instantiateDefaultStateMachine(Artboard, Listener);
    }

    rive::StateMachineHandle CreateStateMachine(
        rive::ArtboardHandle Artboard,
        const FName& StateMachineName,
        rive::CommandQueue::StateMachineListener* Listener = nullptr,
        uint64_t* outRequestId = nullptr)
    {
        FTCHARToUTF8 Convert(*StateMachineName.ToString());
        if (outRequestId)
        {
            *outRequestId = ++CurrentRequestId;
            return CommandQueue->instantiateStateMachineNamed(Artboard,
                                                              Convert.Get(),
                                                              Listener,
                                                              *outRequestId);
        }
        return CommandQueue->instantiateStateMachineNamed(Artboard,
                                                          Convert.Get(),
                                                          Listener);
    }

    rive::StateMachineHandle CreateStateMachine(
        rive::ArtboardHandle Artboard,
        const FString& StateMachineName,
        rive::CommandQueue::StateMachineListener* Listener = nullptr,
        uint64_t* outRequestId = nullptr)
    {
        FTCHARToUTF8 Convert(*StateMachineName);
        if (outRequestId)
        {
            *outRequestId = ++CurrentRequestId;
            return CommandQueue->instantiateStateMachineNamed(Artboard,
                                                              Convert.Get(),
                                                              Listener,
                                                              *outRequestId);
        }
        return CommandQueue->instantiateStateMachineNamed(Artboard,
                                                          Convert.Get(),
                                                          Listener);
    }

    uint64_t StateMachineMouseMove(rive::StateMachineHandle Handle,
                                   rive::CommandQueue::PointerEvent Event)
    {
        CommandQueue->pointerMove(Handle, Event, ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t StateMachineMouseDown(rive::StateMachineHandle Handle,
                                   rive::CommandQueue::PointerEvent Event)
    {
        CommandQueue->pointerDown(Handle, Event, ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t StateMachineMouseOut(rive::StateMachineHandle Handle,
                                  rive::CommandQueue::PointerEvent Event)
    {
        CommandQueue->pointerExit(Handle, Event, ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t StateMachineMouseUp(rive::StateMachineHandle Handle,
                                 rive::CommandQueue::PointerEvent Event)
    {
        CommandQueue->pointerUp(Handle, Event, ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t StateMachineBindViewModel(rive::StateMachineHandle Handle,
                                       rive::ViewModelInstanceHandle ViewModel)
    {
        CommandQueue->bindViewModelInstance(Handle,
                                            ViewModel,
                                            ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t DestroyStateMachine(rive::StateMachineHandle StateMachine)
    {
        CommandQueue->deleteStateMachine(StateMachine, ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t RequestArtboardNames(rive::FileHandle FileHandle)
    {
        CommandQueue->requestArtboardNames(FileHandle, ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t RequestViewModelEnums(rive::FileHandle FileHandle)
    {
        CommandQueue->requestViewModelEnums(FileHandle, ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t RequestViewModelNames(rive::FileHandle FileHandle)
    {
        CommandQueue->requestViewModelNames(FileHandle, ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t RequestViewModelProperties(rive::FileHandle FileHandle,
                                        const FString& ViewModelName)
    {
        FTCHARToUTF8 Convert(*ViewModelName);
        CommandQueue->requestViewModelPropertyDefinitions(FileHandle,
                                                          Convert.Get(),
                                                          ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t RequestViewModelProperties(rive::FileHandle FileHandle,
                                        const std::string& ViewModelName)
    {
        CommandQueue->requestViewModelPropertyDefinitions(FileHandle,
                                                          ViewModelName,
                                                          ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t RequestViewModelInstanceNames(rive::FileHandle FileHandle,
                                           const FString& ViewModelName)
    {
        FTCHARToUTF8 Convert(*ViewModelName);
        CommandQueue->requestViewModelInstanceNames(FileHandle,
                                                    Convert.Get(),
                                                    ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t RequestViewModelInstanceNames(rive::FileHandle FileHandle,
                                           const std::string& ViewModelName)
    {
        CommandQueue->requestViewModelInstanceNames(FileHandle,
                                                    ViewModelName,
                                                    ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t RequestStateMachineNames(rive::ArtboardHandle ArtboardHandle)
    {
        CommandQueue->requestStateMachineNames(ArtboardHandle,
                                               ++CurrentRequestId);
        return CurrentRequestId;
    }

    uint64_t RequestDefaultViewModelInfo(rive::ArtboardHandle ArtboardHandle,
                                         rive::FileHandle FileHandle)
    {
        CommandQueue->requestDefaultViewModelInfo(ArtboardHandle,
                                                  FileHandle,
                                                  ++CurrentRequestId);
        return CurrentRequestId;
    }

    // Queues a callback to be called at the end of the frame. This is more
    // efficient and should be preferred over RunOnceImmediate where possible.
    void RunOnce(ServerSideCallback Callback);
    // Immediately sends a callback to the CommandServer to be run. This should
    // be used when ordering is important.
    void RunOnceImmediate(ServerSideCallback Callback);
    // Request for State Machine advance
    void AdvanceStateMachine(rive::StateMachineHandle, float AdvanceAmount);
    // Enqueues a draw of the given Artboard.
    void DrawArtboard(TSharedPtr<FRiveRenderTarget> RenderTarget,
                      FDrawArtboardCommand);
    // Enqueues a generic draw lambda.
    void Draw(TSharedPtr<FRiveRenderTarget> RenderTarget, DirectDrawCallback);

    // Send all command to the render server.
    void Execute();

private:
    FRiveCommandSet& FindOrAddDrawCommands(
        TSharedPtr<FRiveRenderTarget> RenderTarget)
    {
        auto& RenderTargetDrawCommands = DrawCommands.FindOrAdd(RenderTarget);
        if (RenderTargetDrawCommands.DrawKey == RIVE_NULL_HANDLE)
            RenderTargetDrawCommands.DrawKey = CommandQueue->createDrawKey();
        return RenderTargetDrawCommands;
    }

    static void DrawArtboard(const FDrawArtboardCommand& DrawCommand,
                             rive::CommandServer*,
                             rive::Renderer* Renderer);

    rive::rcp<rive::CommandQueue> CommandQueue;
    // Array of commands that have been enqueued that are not draw commands i.e.
    // they do not need a begin frame / flush.
    TArray<ServerSideCallback> Commands;
    // Map containing draw commands per render target. For performance, we may
    // want to consider std::unordered_map, however this lets us play nicely
    // with UE's garbage collection
    TMap<TSharedPtr<FRiveRenderTarget>, FRiveCommandSet> DrawCommands;

    // Used for data binding external UTextures.
    TMap<TStrongObjectPtr<UTexture2D>, rive::RenderImageHandle> ExternalImages;

    uint64_t CurrentRequestId = 0;
};
