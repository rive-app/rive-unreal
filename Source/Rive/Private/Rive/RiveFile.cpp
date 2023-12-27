// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveFile.h"
#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
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
        UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();

        check(RiveRenderer);

        RiveRenderer->DebugColorDraw(GetRenderTarget(), DebugColor);

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
    if (bIsInitialized == false)
    {
        RenderTarget = UE::Rive::Renderer::FRiveRendererUtils::CreateDefaultRenderTarget({ 1980, 1080 });

        bIsInitialized = true;
    }

    //if (RiveArtboard.IsNull())
    {
        RiveArtboard = NewObject<URiveArtboard>(this);

        RiveArtboard->LoadNativeArtboard(TempFileBuffer);
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
