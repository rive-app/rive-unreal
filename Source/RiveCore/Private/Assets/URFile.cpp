// Copyright Rive, Inc. All rights reserved.

#include "Assets/URFile.h"
#include "Logs/RiveCoreLog.h"

UE::Rive::Assets::FURFile::FURFile(const TArray<uint8>& InBuffer)
    : FURFile(InBuffer.GetData(), InBuffer.Num())
{
}

UE::Rive::Assets::FURFile::FURFile(const uint8* InBytes, uint32 InSize)
#if WITH_RIVE
    : NativeFileSpan(InBytes, InSize)
#endif // WITH_RIVE
{
#if WITH_RIVE
    FileAssetLoader = MakeUnique<FURFileAssetLoader>("");
#endif // WITH_RIVE
}

#if WITH_RIVE

rive::ImportResult UE::Rive::Assets::FURFile::Import(rive::Factory* InRiveFactory, Assets::FURFileAssetLoader* InAssetLoader)
{
    rive::ImportResult ImportResult;

    NativeFilePtr = rive::File::import(NativeFileSpan, InRiveFactory, &ImportResult, InAssetLoader == nullptr ? FileAssetLoader.Get() : InAssetLoader);

    Artboard = MakeUnique<Core::FURArtboard>(NativeFilePtr.get());

    PrintStats();

    return ImportResult;
}

UE::Rive::Core::FURArtboard* UE::Rive::Assets::FURFile::GetArtboard() const
{
    if (!Artboard)
    {
        UE_LOG(LogRiveCore, Error, TEXT("Could not retrieve artboard as we have detected an empty rive file."));

        return nullptr;
    }
   
    return Artboard.Get();
}

void UE::Rive::Assets::FURFile::PrintStats()
{
    if (!NativeFilePtr)
    {
        UE_LOG(LogRiveCore, Error, TEXT("Could not print statistics as we have detected an empty rive file."));

        return;
    }

    FFormatNamedArguments RiveFileLoadArgs;

    RiveFileLoadArgs.Add(TEXT("Major"), FText::AsNumber((int32)NativeFilePtr->majorVersion));

    RiveFileLoadArgs.Add(TEXT("Minor"), FText::AsNumber((int32)NativeFilePtr->minorVersion));

    RiveFileLoadArgs.Add(TEXT("NumArtboards"), FText::AsNumber((uint64)NativeFilePtr->artboardCount()));

    RiveFileLoadArgs.Add(TEXT("NumAssets"), FText::AsNumber((uint64)NativeFilePtr->assets().size()));

    if (const rive::Artboard* NativeArtboard = NativeFilePtr->artboard())
    {
        RiveFileLoadArgs.Add(TEXT("NumAnimations"), FText::AsNumber((uint64)NativeArtboard->animationCount()));
    }
    else
    {
        RiveFileLoadArgs.Add(TEXT("NumAnimations"), FText::AsNumber(0));
    }

    const FText RiveFileLoadMsg = FText::Format(NSLOCTEXT("FURFile", "RiveFileLoadMsg", "Using Rive Runtime : {Major}.{Minor}; Artboard(s) Count : {NumArtboards}; Asset(s) Count : {NumAssets}; Animation(s) Count : {NumAnimations}"), RiveFileLoadArgs);

    UE_LOG(LogRiveCore, Display, TEXT("%s"), *RiveFileLoadMsg.ToString());
}

#endif // WITH_RIVE
