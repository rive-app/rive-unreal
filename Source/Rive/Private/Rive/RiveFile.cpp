// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveFile.h"
#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "IRiveRenderTarget.h"
#include "Rive/RiveArtboard.h"
#include "RiveRendererUtils.h"
#include "Engine/TextureRenderTarget2D.h"

TStatId URiveFile::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(URiveFile, STATGROUP_Tickables);
}

void URiveFile::Tick(float InDeltaSeconds)
{
    if (IsRendering())
    {
        // Maybe only once
        if (bDrawOnceTest == false)
        {
#if WITH_RIVE

            if (rive::Artboard* NativeArtboard = RiveArtboard->GetNativeArtBoard())
            {
                RiveRenderTarget->DrawArtboard((uint8)RiveFitType, RiveAlignment.X, RiveAlignment.Y, NativeArtboard, DebugColor);

                bDrawOnceTest = true;
            }

#endif // WITH_RIVE
        }

        CountdownRenderingTickCounter--;
    }
}

bool URiveFile::IsTickable() const
{
    return FTickableGameObject::IsTickable();
}

void URiveFile::PostLoad()
{
    UObject::PostLoad();

    Initialize();
}

UTextureRenderTarget2D* URiveFile::GetRenderTarget() const
{
    return OverrideRenderTarget != nullptr ? OverrideRenderTarget : RenderTarget;
}

FLinearColor URiveFile::GetDebugColor() const
{
    return DebugColor;
}

void URiveFile::Initialize()
{
    RiveArtboard = NewObject<URiveArtboard>(this);

    RiveArtboard->LoadNativeArtboard(TempFileBuffer);

    if (UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer())
    {
        RenderTarget = RiveRenderer->CreateDefaultRenderTarget({ 800, 1000 });

        RiveRenderTarget = RiveRenderer->CreateTextureTarget_GameThread(*GetPathName(), GetRenderTarget());

        RiveRenderTarget->Initialize();

        bIsInitialized = true;
    }
}

bool URiveFile::IsRendering() const
{
    return CountdownRenderingTickCounter > 0;
}

void URiveFile::RequestRendering()
{
    CountdownRenderingTickCounter = CountdownRenderingTicks;
}

void URiveFile::SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass)
{
    WidgetClass = InWidgetClass;
}
