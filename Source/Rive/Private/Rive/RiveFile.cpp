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
    // if (IsRendering())
    // {
    //     // Maybe only once
    //     //if (bDrawOnceTest == false)
    //     {
    //        RiveRenderTarget->DrawArtboard(TEnumAsByte<ERiveFitType>(RiveFitType).GetIntValue(), RiveAlignment.X, RiveAlignment.Y, RiveArtboard->GetNativeArtBoard());
    //         bDrawOnceTest = true;
    //     }
    //
    //     CountdownRenderingTickCounter--;
    // }
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
    UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
    RenderTarget = RiveRenderer->CreateDefaultRenderTarget({ 800, 1000 });
    RiveRenderTarget = RiveRenderer->CreateTextureTarget_GameThread(*GetPathName(), GetRenderTarget());
    RiveRenderTarget->Initialize();

    //if (bIsInitialized == false)
    {

        // Not the best solution, just for testing we can do here. The problem that won't cover the user update texture in UI,
        // we would need to update texture in renderer
        check(RiveRenderer);
        bIsInitialized = true;
    }

    FCoreDelegates::OnEndFrame.AddLambda([this]()
    {
        if (IsRendering())
        {
            //if (bDrawOnceTest == false)
            {
               RiveRenderTarget->DrawArtboard(TEnumAsByte<ERiveFitType>(RiveFitType).GetIntValue(), RiveAlignment.X, RiveAlignment.Y, RiveArtboard->GetNativeArtBoard());
                bDrawOnceTest = true;
            }

            CountdownRenderingTickCounter--;
        }
    });
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
