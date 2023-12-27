// Copyright Rive, Inc. All rights reserved.

#include "Assets/UnrealRiveFile.h"
#include "Logs/RiveCoreLog.h"

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

rive::ImportResult UE::Rive::Assets::FUnrealRiveFile::Import(Core::FUnrealRiveFactory* InRiveFactory, Assets::FUnrealRiveFileAssetLoader* InAssetLoader)
{
    rive::ImportResult ImportResult;

    NativeFilePtr = rive::File::import(NativeFileSpan, InRiveFactory, &ImportResult, InAssetLoader == nullptr ? FileAssetLoader.Get() : InAssetLoader);

    PrintStats();

    return ImportResult;
}

rive::Artboard* UE::Rive::Assets::FUnrealRiveFile::GetNativeArtBoard()
{
    return NativeFilePtr->artboard();
}

void UE::Rive::Assets::FUnrealRiveFile::PrintStats()
{
    if (!NativeFilePtr)
    {
        UE_LOG(LogRiveCore, Error, TEXT("Empty rive file detected."));

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
