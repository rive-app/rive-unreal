// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "Rive/RiveRenderTargetUpdater.h"

#include "Kismet/GameplayStatics.h"
#include "Rive/RiveArtboard.h"
#include "Rive/RiveRenderTarget2D.h"

#include "Engine/World.h"

// Sets default values for this component's properties
URiveRenderTargetUpdater::URiveRenderTargetUpdater()
{
    // Set this component to be initialized when the game starts, and to be
    // ticked every frame.  You can turn these features off to improve
    // performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts
void URiveRenderTargetUpdater::BeginPlay()
{
    Super::BeginPlay();
    if (bEnableMouseEvents)
    {
        auto PlayerController =
            UGameplayStatics::GetPlayerController(this, PlayerIndex);

        GetOwner()->OnBeginCursorOver.AddDynamic(
            this,
            &URiveRenderTargetUpdater::OnMouseEnter);
        GetOwner()->OnEndCursorOver.AddDynamic(
            this,
            &URiveRenderTargetUpdater::OnMouseLeave);
        GetOwner()->OnClicked.AddDynamic(
            this,
            &URiveRenderTargetUpdater::OnMouseDown);
        GetOwner()->OnReleased.AddDynamic(this,
                                          &URiveRenderTargetUpdater::OnMouseUp);
    }
}

// Called every frame
void URiveRenderTargetUpdater::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (IsValid(RenderTargetToUpdate))
    {
        if (bAutoDrawRenderTarget)
        {
            RenderTargetToUpdate->Draw();
        }

        if (bEnableMouseEvents && bIsMouseWithinView)
        {
            FVector2D UV;
            if (LineTraceWithUVResult(UV))
            {
                auto Artboard = RenderTargetToUpdate->GetArtboard();
                Artboard->PointerMove(RenderTargetToUpdate->RiveDescriptor, UV);
            }
        }
    }
}

void URiveRenderTargetUpdater::OnMouseEnter(AActor* TouchedActor)
{
    bIsMouseWithinView = true;
}

void URiveRenderTargetUpdater::OnMouseLeave(AActor* TouchedActor)
{
    if (bIsMouseWithinView)
    {
        FVector2D UV;
        if (LineTraceWithUVResult(UV))
        {
            auto Artboard = RenderTargetToUpdate->GetArtboard();
            Artboard->PointerExit(RenderTargetToUpdate->RiveDescriptor, UV);
        }
    }
    bIsMouseWithinView = false;
}

void URiveRenderTargetUpdater::OnMouseUp(AActor* TouchedActor,
                                         FKey ButtonPressed)
{
    if (ButtonPressed == EKeys::LeftMouseButton)
    {
        FVector2D UV;
        if (LineTraceWithUVResult(UV))
        {
            auto Artboard = RenderTargetToUpdate->GetArtboard();
            Artboard->PointerUp(RenderTargetToUpdate->RiveDescriptor, UV);
        }
    }
}

void URiveRenderTargetUpdater::OnMouseDown(AActor* TouchedActor,
                                           FKey ButtonPressed)
{
    if (ButtonPressed == EKeys::LeftMouseButton)
    {
        FVector2D UV;
        if (LineTraceWithUVResult(UV))
        {
            auto Artboard = RenderTargetToUpdate->GetArtboard();
            Artboard->PointerDown(RenderTargetToUpdate->RiveDescriptor, UV);
        }
    }
}

bool URiveRenderTargetUpdater::LineTraceWithUVResult(FVector2D& OutUVHit)
{
    auto PlayerController =
        UGameplayStatics::GetPlayerController(this, PlayerIndex);
    if (IsValid(PlayerController))
    {
        FVector WorldLocation;
        FVector WorldDirection;
        if (PlayerController->DeprojectMousePositionToWorld(WorldLocation,
                                                            WorldDirection))
        {
            FHitResult HitResult;
            FCollisionQueryParams Params(FName(
                TEXT("Rive.URiveRenderTargetUpdater.LineTraceWithUVResult")));
            Params.bTraceComplex = true;
            Params.bReturnFaceIndex = true;
            if (GetWorld()->LineTraceSingleByChannel(
                    HitResult,
                    WorldLocation,
                    WorldLocation + WorldDirection * LineTraceDistance,
                    ECC_Visibility,
                    Params))
            {
                if (HitResult.IsValidBlockingHit() &&
                    UGameplayStatics::FindCollisionUV(HitResult,
                                                      UVChannel,
                                                      OutUVHit))
                {
                    if (bInvertUVX)
                    {
                        OutUVHit.X = 1.0 - OutUVHit.X;
                    }

                    if (bInvertUVY)
                    {
                        OutUVHit.Y = 1.0 - OutUVHit.Y;
                    }

                    return true;
                }
            }
        }
    }

    return false;
}
