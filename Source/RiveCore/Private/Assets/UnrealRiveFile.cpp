// Copyright Rive, Inc. All rights reserved.

#include "Assets/UnrealRiveFile.h"
#include "Logs/RiveCoreLog.h"
#include "rive/animation/state_machine_instance.hpp"

UE::Rive::Assets::FUnrealRiveFile::FUnrealRiveFile(const TArray<uint8>& InBuffer)
    : FUnrealRiveFile(InBuffer.GetData(), InBuffer.Num())
{
}

UE::Rive::Assets::FUnrealRiveFile::FUnrealRiveFile(const uint8* InBytes, uint32 InSize)
#if WITH_RIVE
    : NativeFileSpan(InBytes, InSize)
#endif // WITH_RIVE
{
    FileAssetLoader = MakeUnique<FUnrealRiveFileAssetLoader>("");
}

#if WITH_RIVE

rive::ImportResult UE::Rive::Assets::FUnrealRiveFile::Import(rive::Factory* InRiveFactory, Assets::FUnrealRiveFileAssetLoader* InAssetLoader)
{
    rive::ImportResult ImportResult;

    NativeFilePtr = rive::File::import(NativeFileSpan, InRiveFactory, &ImportResult, InAssetLoader == nullptr ? FileAssetLoader.Get() : InAssetLoader);

    if (rive::Artboard* NativeArtboard = NativeFilePtr->artboard())
    {
        ///NativeArtboard->advance(0);

        ArtboardInstance = NativeArtboard->instance();
        ArtboardInstance->advance(0);

        // Move State machine to separate file
        std::unique_ptr<rive::StateMachineInstance> DefaultStateMachine = ArtboardInstance->defaultStateMachine();
        if (DefaultStateMachine == nullptr)
        {
            DefaultStateMachine = ArtboardInstance->stateMachineAt(0);
        }
        if (DefaultStateMachine)
        {
            DefaultStateMachinePtr = DefaultStateMachine.release();
        }
    }
    

    PrintStats();

    return ImportResult;
}

rive::Artboard* UE::Rive::Assets::FUnrealRiveFile::GetNativeArtBoard() const
{
    // if (!NativeFilePtr)
    // {
    //     UE_LOG(LogRiveCore, Error, TEXT("Could not retrieve artboard as we have detected an empty rive file."));
    //
    //     return nullptr;
    // }
    //
    // return NativeFilePtr->artboard();

    return ArtboardInstance.get();
}

FVector2f UE::Rive::Assets::FUnrealRiveFile::GetArtboardSize() const
{
    // Have checks here
    const rive::Artboard* NativArtboard = GetNativeArtBoard();
    return {NativArtboard->width(), NativArtboard->height() };
}

UE_DISABLE_OPTIMIZATION
void UE::Rive::Assets::FUnrealRiveFile::AdvanceDefaultStateMachine(const float inSeconds)
{
    if (DefaultStateMachinePtr)
    {
        DefaultStateMachinePtr->advanceAndApply(inSeconds);
    }
}
UE_ENABLE_OPTIMIZATION

void UE::Rive::Assets::FUnrealRiveFile::PrintStats()
{
    if (!NativeFilePtr)
    {
        UE_LOG(LogRiveCore, Error, TEXT("Could not print statistics as we have detected an empty rive file."));

        return;
    }

    FFormatNamedArguments RiveFileLoadArgs;

    RiveFileLoadArgs.Add(TEXT("Major"), FText::AsNumber(NativeFilePtr->majorVersion));

    RiveFileLoadArgs.Add(TEXT("Minor"), FText::AsNumber(NativeFilePtr->minorVersion));

    RiveFileLoadArgs.Add(TEXT("NumArtboards"), FText::AsNumber(NativeFilePtr->artboardCount()));

    RiveFileLoadArgs.Add(TEXT("NumAssets"), FText::AsNumber(NativeFilePtr->assets().size()));

    if (const rive::Artboard* ArtBoard = NativeFilePtr->artboard())
    {
        RiveFileLoadArgs.Add(TEXT("NumAnimations"), FText::AsNumber(ArtBoard->animationCount()));
    }
    else
    {
        RiveFileLoadArgs.Add(TEXT("NumAnimations"), FText::AsNumber(0));
    }

    const FText RiveFileLoadMsg = FText::Format(NSLOCTEXT("FUnrealRiveFile", "RiveFileLoadMsg", "Using Rive Runtime : {Major}.{Minor}; Artboard(s) Count : {NumArtboards}; Asset(s) Count : {NumAssets}; Animation(s) Count : {NumAnimations}"), RiveFileLoadArgs);

    UE_LOG(LogRiveCore, Display, TEXT("%s"), *RiveFileLoadMsg.ToString());
}

#endif // WITH_RIVE
