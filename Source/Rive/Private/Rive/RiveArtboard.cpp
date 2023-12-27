// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveArtboard.h"
#include "IRiveCoreModule.h"
#include "Logs/RiveLog.h"

#pragma warning(push)
#pragma warning(disable: 4458)

UE_DISABLE_OPTIMIZATION

bool URiveArtboard::LoadNativeArtboard(const TArray<uint8>& InBuffer)
{
    if (!UE::Rive::Core::IRiveCoreModule::IsAvailable())
    {
        UE_LOG(LogRive, Error, TEXT("Could not load native artboard as the required Rive Core Module is either missing or not loaded properly."));

        return false;
    }

    UE::Rive::Core::FUnrealRiveFactory& UnrealRiveFactory = UE::Rive::Core::IRiveCoreModule::Get().GetRiveFactory();

    if (!UnrealRiveFile.IsValid())
    {
        UnrealRiveFile = MakeUnique<UE::Rive::Assets::FUnrealRiveFile>(InBuffer.GetData(), InBuffer.Num());
    }

#if WITH_RIVE

    rive::ImportResult ImportResult = UnrealRiveFile->Import(&UnrealRiveFactory);

    if (ImportResult != rive::ImportResult::success)
    {
        UE_LOG(LogRive, Error, TEXT("Failed to import rive file."));
        
        return false;
    }

#endif // WITH_RIVE

    return true;
}

rive::Artboard* URiveArtboard::GetNativeArtBoard()
{
   return UnrealRiveFile->GetNativeArtBoard();
}

UE_ENABLE_OPTIMIZATION

#pragma warning(pop)