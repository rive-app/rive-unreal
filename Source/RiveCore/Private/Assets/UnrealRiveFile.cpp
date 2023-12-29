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

rive::ImportResult UE::Rive::Assets::FUnrealRiveFile::Import(rive::Factory* InRiveFactory, Assets::FUnrealRiveFileAssetLoader* InAssetLoader)
{
    rive::ImportResult ImportResult;

    NativeFilePtr = rive::File::import(NativeFileSpan, InRiveFactory, &ImportResult, InAssetLoader == nullptr ? FileAssetLoader.Get() : InAssetLoader);

    if (rive::Artboard* NativeArtboard = NativeFilePtr->artboard())
    {
        NativeArtboard->advance(0);
    }

    /*rive::StatusCode StatusCode = Artboard->initialize();

    if (StatusCode != rive::StatusCode::Ok)
    {
        UE_LOG(LogTemp, Warning, TEXT("not initialize"));
    }*/

    PrintStats();

    return ImportResult;
}

rive::Artboard* UE::Rive::Assets::FUnrealRiveFile::GetNativeArtBoard()
{
    if (!NativeFilePtr)
    {
        UE_LOG(LogRiveCore, Error, TEXT("Could not retrieve artboard as we have detected an empty rive file."));

        return nullptr;
    }

    return NativeFilePtr->artboard();
}

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
