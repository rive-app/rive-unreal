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

        // Pass rive art board just for testing, that is wrong, just for testing, we should not pass native artboard here
        RiveRenderer->DebugColorDraw(GetRenderTarget(), DebugColor, RiveArtboard->GetNativeArtBoard());

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
    //if (RiveArtboard.IsNull())
    {
        RiveArtboard = NewObject<URiveArtboard>(this);

        RiveArtboard->LoadNativeArtboard(TempFileBuffer);
    }

    RenderTarget = UE::Rive::Renderer::FRiveRendererUtils::CreateDefaultRenderTarget({ 800, 1000 }, EPixelFormat::PF_R8G8B8A8, true);

    if (bIsInitialized == false)
    {

        // Not the best solution, just for testing we can do here. The problem that won't cover the user update texture in UI,
        // we would need to update texture in renderer
        UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
        check(RiveRenderer);

        RiveRenderer->SetTextureTarget_GameThread(*GetPathName(), GetRenderTarget());

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
