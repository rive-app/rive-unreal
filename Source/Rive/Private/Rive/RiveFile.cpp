// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveFile.h"
#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "IRiveRenderTarget.h"
#include "Logs/RiveLog.h"
#include "RiveRendererUtils.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/pls/pls_render_context.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

URiveFile::URiveFile()
{
    OverrideFormat = EPixelFormat::PF_R8G8B8A8;

    bCanCreateUAV = true;

    // TODO. We might need extra texture to Draw rive separately and copy to this

    SizeX = 500;

    SizeY = 500;
}

TStatId URiveFile::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(URiveFile, STATGROUP_Tickables);
}

UE_DISABLE_OPTIMIZATION

void URiveFile::Tick(float InDeltaSeconds)
{
    if (bIsInitialized && bIsRendering)
    {
        // Maybe only once
        //if (bDrawOnceTest == false)
        {
#if WITH_RIVE

            if (const UE::Rive::Core::FURArtboard* Artboard = GetArtboard())
            {
                if (UE::Rive::Core::FURStateMachine* StateMachine = Artboard->GetStateMachine())
                {
                    if (!bIsReceivingInput)
                    {
                        StateMachine->Advance(InDeltaSeconds);
                    }
                }

                const FVector2f RiveAlignmentXY = GetRiveAlignment();

                RiveRenderTarget->AlignArtboard((uint8)RiveFitType, RiveAlignmentXY.X, RiveAlignmentXY.Y, Artboard->GetNativeArtboard(), DebugColor);

                RiveRenderTarget->DrawArtboard(Artboard->GetNativeArtboard(), DebugColor);

                bDrawOnceTest = true;
            }
            
            // Copy from render target
            // TODO. move from here
            // Separate target might be needed to let Rive draw only to separate texture
            {
                FTextureRenderTargetResource* RiveFileResource = GameThread_GetRenderTargetResource();

                FTextureRenderTargetResource* RiveFileRenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();

                ENQUEUE_RENDER_COMMAND(CopyRenderTexture)(
                    [this, RiveFileResource, RiveFileRenderTargetResource](FRHICommandListImmediate& RHICmdList)
                    {
                        UE::Rive::Renderer::FRiveRendererUtils::CopyTextureRDG(RHICmdList, RiveFileRenderTargetResource->TextureRHI, RiveFileResource->TextureRHI);
                    });
            }

#endif // WITH_RIVE
        }
    }
}

UE_ENABLE_OPTIMIZATION

bool URiveFile::IsTickable() const
{
    return FTickableGameObject::IsTickable();
}

uint32 URiveFile::CalcTextureMemorySizeEnum(ETextureMipCount Enum) const
{
    // Calculate size based on format.  All mips are resident on render targets so we always return the same value.
    EPixelFormat Format = GetFormat();

    int32 BlockSizeX = GPixelFormats[Format].BlockSizeX;

    int32 BlockSizeY = GPixelFormats[Format].BlockSizeY;

    int32 BlockBytes = GPixelFormats[Format].BlockBytes;

    int32 NumBlocksX = (SizeX + BlockSizeX - 1) / BlockSizeX;

    int32 NumBlocksY = (SizeY + BlockSizeY - 1) / BlockSizeY;

    int32 NumBytes = NumBlocksX * NumBlocksY * BlockBytes;

    return NumBytes;
}

void URiveFile::PostLoad()
{
    UObject::PostLoad();

    Initialize();
}

#if WITH_EDITOR

void URiveFile::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    // TODO. WE need custom implementation here to handle the Rive File Editor Changes
}

#endif // WITH_EDITOR

void URiveFile::FireTrigger(const FString& InPropertyName) const
{
#if WITH_RIVE

    if (const UE::Rive::Core::FURArtboard* Artboard = GetArtboard())
    {
        if (UE::Rive::Core::FURStateMachine* StateMachine = Artboard->GetStateMachine())
        {
            StateMachine->FireTrigger(InPropertyName);
        }
    }

#endif // WITH_RIVE
}

bool URiveFile::GetBoolValue(const FString& InPropertyName) const
{
#if WITH_RIVE

    if (const UE::Rive::Core::FURArtboard* Artboard = GetArtboard())
    {
        if (UE::Rive::Core::FURStateMachine* StateMachine = Artboard->GetStateMachine())
        {
            return StateMachine->GetBoolValue(InPropertyName);
        }
    }

    return false;

#else

    return false;

#endif // !WITH_RIVE
}

bool URiveFile::GetCustomBoolValue(const FString& InEventName, const FString& InPropertyName) const
{
    return false;
}

float URiveFile::GetCustomNumberValue(const FString& InEventName, const FString& InPropertyName) const
{
    return 0.0f;
}

const FString URiveFile::GetCustomStringValue(const FString& InEventName, const FString& InPropertyName) const
{
    return FString();
}

float URiveFile::GetNumberValue(const FString& InPropertyName) const
{
#if WITH_RIVE

    if (const UE::Rive::Core::FURArtboard* Artboard = GetArtboard())
    {
        if (UE::Rive::Core::FURStateMachine* StateMachine = Artboard->GetStateMachine())
        {
            return StateMachine->GetNumberValue(InPropertyName);
        }
    }

    return 0.f;

#else

    return 0.f;

#endif // !WITH_RIVE
}

FLinearColor URiveFile::GetDebugColor() const
{
    return DebugColor;
}

FVector2f URiveFile::GetLocalCoordinates(const FVector2f& InScreenPosition, const FBox2f& InScreenRect, const FIntPoint& InViewportSize) const
{
#if WITH_RIVE

    if (const UE::Rive::Core::FURArtboard* Artboard = GetArtboard())
    {
        const FVector2f RiveAlignmentXY = GetRiveAlignment();

        const FIntPoint TextureSize = CalculateRenderTextureSize(InViewportSize);
        
        const FVector2f TexturePosition = InScreenRect.Min + CalculateRenderTexturePosition(InViewportSize, TextureSize);

        const rive::Mat2D Transform = rive::computeAlignment((rive::Fit)RiveFitType,
            rive::Alignment(RiveAlignmentXY.X, RiveAlignmentXY.Y),
            rive::AABB(
                TexturePosition.X,
                TexturePosition.Y,
                TexturePosition.X + TextureSize.X,
                TexturePosition.Y + TextureSize.Y),
            Artboard->GetBounds());
    
        const rive::Vec2D ResultingVector = Transform.invertOrIdentity() * rive::Vec2D(InScreenPosition.X, InScreenPosition.Y);
    
        return { ResultingVector.x, ResultingVector.y };
    }

#endif // WITH_RIVE

    return FVector2f::ZeroVector;
}

void URiveFile::SetBoolValue(const FString& InPropertyName, bool bNewValue)
{
#if WITH_RIVE

    if (const UE::Rive::Core::FURArtboard* Artboard = GetArtboard())
    {
        if (UE::Rive::Core::FURStateMachine* StateMachine = Artboard->GetStateMachine())
        {
            StateMachine->SetBoolValue(InPropertyName, bNewValue);
        }
    }

#endif // WITH_RIVE
}

void URiveFile::SetNumberValue(const FString& InPropertyName, float NewValue)
{
#if WITH_RIVE

    if (const UE::Rive::Core::FURArtboard* Artboard = GetArtboard())
    {
        if (UE::Rive::Core::FURStateMachine* StateMachine = Artboard->GetStateMachine())
        {
            StateMachine->SetNumberValue(InPropertyName, NewValue);
        }
    }

#endif // WITH_RIVE
}

FIntPoint URiveFile::CalculateRenderTextureSize(const FIntPoint& InViewportSize) const
{
    FIntPoint NewSize = { SizeX, SizeY };

    const FVector2f ArtboardSize = GetArtboard()->GetSize();
    const float TextureAspectRatio = ArtboardSize.X / ArtboardSize.Y;
    
    switch (RiveFitType)
    {
        case ERiveFitType::Fill:
            NewSize = InViewportSize;
            break;
        case ERiveFitType::Contain:
            // Ensures one dimension takes up the container space fully, without clipping
            if (InViewportSize.X > InViewportSize.Y)
            {
                NewSize = {
                    static_cast<int>(InViewportSize.Y / TextureAspectRatio),
                    InViewportSize.Y
                };
            } else
            {
                NewSize = {
                    static_cast<int>(InViewportSize.X / TextureAspectRatio),
                    InViewportSize.X
                };
            }
            break;
        case ERiveFitType::Cover:
            {
                // This Ensures one of the dimensions will always take up container space fully, with clipping
                if (InViewportSize.X > InViewportSize.Y)
                {
                    NewSize = {
                        InViewportSize.X,
                        static_cast<int>(InViewportSize.X / TextureAspectRatio)
                    };
                } else
                {
                    NewSize = {
                        static_cast<int>(InViewportSize.Y / TextureAspectRatio),
                        InViewportSize.Y
                    };
                }
                break;
            }
        case ERiveFitType::FitWidth:
            // Force Width to take up the screenspace width; Maintain aspect ratio, can clip
            NewSize = {
                InViewportSize.X,
                static_cast<int>(InViewportSize.X / TextureAspectRatio)
            };
            break;
        case ERiveFitType::FitHeight:
            // Force to take up the screenspace height; Maintain aspect ratio, can clip
            NewSize = {
                static_cast<int>(InViewportSize.Y / TextureAspectRatio),
                InViewportSize.Y
            };
            break;
        case ERiveFitType::None:
            NewSize = { SizeX, SizeY };
            break;
        case ERiveFitType::ScaleDown:
            // Take up maximum texture size, or scale it down
            if (InViewportSize.X < SizeX)
            {
                NewSize = {
                    InViewportSize.X,
                    static_cast<int>(InViewportSize.X / TextureAspectRatio)
                };
            } else if (InViewportSize.Y < SizeY)
            {
                NewSize = {
                    InViewportSize.Y,
                    static_cast<int>(InViewportSize.Y / TextureAspectRatio)
                };
            }
            break;
    }
   
    return NewSize;
}

FIntPoint URiveFile::CalculateRenderTexturePosition(const FIntPoint& InViewportSize, const FIntPoint& InTextureSize) const
{
    FIntPoint NewPosition = FIntPoint::ZeroValue;

    const FIntPoint TextureSize = { InTextureSize.X, InTextureSize.Y };

    if (RiveFitType == ERiveFitType::Fill)
    {
        return NewPosition;
    }

    ERiveAlignment Alignment = RiveAlignment;

    switch (Alignment)
    {
        case ERiveAlignment::TopLeft:
            break;
        case ERiveAlignment::TopCenter:
        {
            const int32 PosX = (InViewportSize.X - TextureSize.X) * 0.5f;

            NewPosition = { PosX, 0 };

            break;
        }
        case ERiveAlignment::TopRight:
        {
            const int32 PosX = InViewportSize.X - TextureSize.X;

            NewPosition = { PosX, 0 };

            break;
        }
        case ERiveAlignment::CenterLeft:
        {
            const int32 PosY = (InViewportSize.Y - TextureSize.Y) * 0.5f;

            NewPosition = { 0, PosY };

            break;
        }
        case ERiveAlignment::Center:
        {
            const int32 PosX = (InViewportSize.X - TextureSize.X) * 0.5f;

            const int32 PosY = (InViewportSize.Y - TextureSize.Y) * 0.5f;

            NewPosition = { PosX, PosY };

            break;
        }
        case ERiveAlignment::CenterRight:
        {
            const int32 PosX = InViewportSize.X - TextureSize.X;

            const int32 PosY = (InViewportSize.Y - TextureSize.Y) * 0.5f;

            NewPosition = { PosX, PosY };

            break;
        }
        case ERiveAlignment::BottomLeft:
        {
            const int32 PosY = InViewportSize.Y - TextureSize.Y;

            NewPosition = { 0, PosY };

            break;
        }
        case ERiveAlignment::BottomCenter:
        {
            const int32 PosX = (InViewportSize.X - TextureSize.X) * 0.5f;

            const int32 PosY = InViewportSize.Y - TextureSize.Y;

            NewPosition = { PosX, PosY };

            break;
        }
        case ERiveAlignment::BottomRight:
        {
            const int32 PosX = InViewportSize.X - TextureSize.X;

            const int32 PosY = InViewportSize.Y - TextureSize.Y;

            NewPosition = { PosX, PosY };

            break;
        }
    }

    return NewPosition;
}

FVector2f URiveFile::GetRiveAlignment() const
{
    FVector2f NewAlignment = FRiveAlignment::Center;

    switch (RiveAlignment)
    {
        case ERiveAlignment::TopLeft:
            NewAlignment = FRiveAlignment::TopLeft;
            break;
        case ERiveAlignment::TopCenter:
            NewAlignment = FRiveAlignment::TopCenter;
            break;
        case ERiveAlignment::TopRight:
            NewAlignment = FRiveAlignment::TopRight;
            break;
        case ERiveAlignment::CenterLeft:
            NewAlignment = FRiveAlignment::CenterLeft;
            break;
        case ERiveAlignment::Center:
            break;
        case ERiveAlignment::CenterRight:
            NewAlignment = FRiveAlignment::CenterRight;
            break;
        case ERiveAlignment::BottomLeft:
            NewAlignment = FRiveAlignment::BottomLeft;
            break;
        case ERiveAlignment::BottomCenter:
            NewAlignment = FRiveAlignment::BottomCenter;
            break;
        case ERiveAlignment::BottomRight:
            NewAlignment = FRiveAlignment::BottomRight;
            break;
    }

    return NewAlignment;
}

ESimpleElementBlendMode URiveFile::GetSimpleElementBlendMode() const
{
    ESimpleElementBlendMode NewBlendMode = ESimpleElementBlendMode::SE_BLEND_Opaque;

    switch (RiveBlendMode)
    {
        case ERiveBlendMode::SE_BLEND_Opaque:
            break;
        case ERiveBlendMode::SE_BLEND_Masked:
            NewBlendMode = SE_BLEND_Masked;
            break;
        case ERiveBlendMode::SE_BLEND_Translucent:
            NewBlendMode = SE_BLEND_Translucent;
            break;
        case ERiveBlendMode::SE_BLEND_Additive:
            NewBlendMode = SE_BLEND_Additive;
            break;
        case ERiveBlendMode::SE_BLEND_Modulate:
            NewBlendMode = SE_BLEND_Modulate;
            break;
        case ERiveBlendMode::SE_BLEND_MaskedDistanceField:
            NewBlendMode = SE_BLEND_MaskedDistanceField;
            break;
        case ERiveBlendMode::SE_BLEND_MaskedDistanceFieldShadowed:
            NewBlendMode = SE_BLEND_MaskedDistanceFieldShadowed;
            break;
        case ERiveBlendMode::SE_BLEND_TranslucentDistanceField:
            NewBlendMode = SE_BLEND_TranslucentDistanceField;
            break;
        case ERiveBlendMode::SE_BLEND_TranslucentDistanceFieldShadowed:
            NewBlendMode = SE_BLEND_TranslucentDistanceFieldShadowed;
            break;
        case ERiveBlendMode::SE_BLEND_AlphaComposite:
            NewBlendMode = SE_BLEND_AlphaComposite;
            break;
        case ERiveBlendMode::SE_BLEND_AlphaHoldout:
            NewBlendMode = SE_BLEND_AlphaHoldout;
            break;
    }

    return NewBlendMode;
}

void URiveFile::Initialize()
{
    if (!LoadNativeFile())
    {
        UE_LOG(LogRive, Error, TEXT("Unable to load the native artboard. Check log for more details."));

        return;
    }

    if (UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer())
    {
        if (UE::Rive::Core::FURArtboard* Artboard = GetArtboard())
        {
            const FVector2f ArtboardSize = Artboard->GetSize();

            constexpr bool bInForceLinearGamma = false;

            InitCustomFormat(ArtboardSize.X, ArtboardSize.Y, OverrideFormat, bInForceLinearGamma);

            RenderTarget = RiveRenderer->CreateDefaultRenderTarget(FIntPoint(ArtboardSize.X, ArtboardSize.Y));

            FlushRenderingCommands();

            RiveRenderTarget = RiveRenderer->CreateTextureTarget_GameThread(*GetPathName(), RenderTarget);

            RiveRenderTarget->Initialize();

            bIsInitialized = true;
        }
    }
}

void URiveFile::SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass)
{
    WidgetClass = InWidgetClass;
}

UE_DISABLE_OPTIMIZATION

UE::Rive::Core::FURArtboard* URiveFile::GetArtboard() const
{
    if (!UnrealRiveFile)
    {
        UE_LOG(LogRive, Error, TEXT("Could not retrieve native artboard as we have detected an empty rive file."));

        return nullptr;
    }

    return UnrealRiveFile->GetArtboard();
}

bool URiveFile::LoadNativeFile()
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
        UnrealRiveFile = MakeUnique<UE::Rive::Assets::FURFile>(TempFileBuffer.GetData(), TempFileBuffer.Num());
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

UE_ENABLE_OPTIMIZATION
