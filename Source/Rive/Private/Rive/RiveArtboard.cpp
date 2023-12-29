// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveArtboard.h"
#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "Logs/RiveLog.h"

#pragma warning(push)
#pragma warning(disable: 4458)

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/pls/pls_render_context.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

UE_DISABLE_OPTIMIZATION

bool URiveArtboard::LoadNativeArtboard(const TArray<uint8>& InBuffer)
{
    if (!UE::Rive::Renderer::IRiveRendererModule::IsAvailable())
    {
        UE_LOG(LogRive, Error, TEXT("Could not load native artboard as the required Rive Renderer Module is either missing or not loaded properly."));

        return false;
    }

    UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();

    if (!RiveRenderer)
    {
        UE_LOG(LogRive, Error, TEXT("Failed to import rive file as we do not have a valid renderer."));

        return false;
    }

    if (!RiveRenderer->IsInitialized())
    {
        UE_LOG(LogRive, Error, TEXT("Could not load native artboard as the required Rive Renderer is not initialized."));

        return false;
    }

    if (!UnrealRiveFile.IsValid())
    {
        UnrealRiveFile = MakeUnique<UE::Rive::Assets::FUnrealRiveFile>(InBuffer.GetData(), InBuffer.Num());
    }

#if WITH_RIVE

    if (rive::pls::PLSRenderContext* PLSRenderContext = RiveRenderer->GetPLSRenderContextPtr())
    {
        rive::ImportResult ImportResult = UnrealRiveFile->Import(PLSRenderContext);

        if (ImportResult != rive::ImportResult::success)
        {
            UE_LOG(LogRive, Error, TEXT("Failed to import rive file."));

            return false;
        }
    }
    else
    {
        UE_LOG(LogRive, Error, TEXT("Failed to import rive file as we do not have a valid context."));

        return false;
    }

#endif // WITH_RIVE

    return true;
}

rive::Artboard* URiveArtboard::GetNativeArtBoard()
{
   return UnrealRiveFile->GetNativeArtBoard();
}

void URiveArtboard::AdvanceDefaultStateMachine(const float inSeconds)
{
    UnrealRiveFile->AdvanceDefaultStateMachine(inSeconds);
}

FVector2f URiveArtboard::GetArtboardSize() const
{
    return UnrealRiveFile->GetArtboardSize();
}

UE_ENABLE_OPTIMIZATION

#pragma warning(pop)