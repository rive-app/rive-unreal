// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveFile.h"
#include "IRiveRenderTarget.h"
#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "Logs/RiveLog.h"
#include "Rive/Assets/RiveAsset.h"
#include "Rive/Assets/URAssetImporter.h"
#include "Rive/Assets/URFileAssetLoader.h"
#include "RiveRendererUtils.h"
#include "Rive/RiveArtboard.h"


#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/pls/pls_render_context.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

URiveFile::URiveFile()
{
    OverrideFormat = EPixelFormat::PF_R8G8B8A8;

    bCanCreateUAV = false;

    // TODO. We might need extra texture to Draw rive separately and copy to this

    SizeX = 500;

    SizeY = 500;

    ArtboardIndex = 0;
}

URiveFile* URiveFile::CreateInstance(UObject* InOuter)
{
    auto NewRiveFileInstance = NewObject<URiveFile>(InOuter, URiveFile::StaticClass(), NAME_None, RF_Public | RF_Transient);
    NewRiveFileInstance->ParentRiveFile = this;
    NewRiveFileInstance->Initialize();
    return NewRiveFileInstance;
}

TStatId URiveFile::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(URiveFile, STATGROUP_Tickables);
}

void URiveFile::Tick(float InDeltaSeconds)
{
    if (!IsValidChecked(this)) return;
    
#if WITH_RIVE
    if (!bIsInitialized && bIsFileImported && GetArtboard())
    {
        UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();

        const FVector2f ArtboardSize = Artboard->GetSize();
#if PLATFORM_ANDROID
        constexpr bool bInForceLinearGamma = true; // needed to be true for Android
#else
        constexpr bool bInForceLinearGamma = false; // needed to be true for Android
#endif
        
        OverrideFormat = PF_R8G8B8A8;
        SRGB = false;
        RenderTargetFormat = RTF_RGBA8;
        InitCustomFormat(ArtboardSize.X, ArtboardSize.Y, OverrideFormat, bInForceLinearGamma);
        UpdateResourceImmediate(true);
        
        RenderTarget = RiveRenderer->CreateDefaultRenderTarget(FIntPoint(ArtboardSize.X, ArtboardSize.Y));
        FlushRenderingCommands();
        RiveRenderTarget = RiveRenderer->CreateTextureTarget_GameThread(*GetPathName(), GetRenderTargetToDrawOnto());
        RiveRenderTarget->Initialize();

        bIsInitialized = true;
    }
    if (bIsInitialized && bIsRendering)
    {
        // Empty reported events at the beginning
        TickRiveReportedEvents.Empty();
        if (GetArtboard())
        {
            UE::Rive::Core::FURStateMachine* StateMachine = Artboard->GetStateMachine();
            if (StateMachine && StateMachine->IsValid())
            {
                FScopeLock Lock(&RiveRenderTarget->GetThreadDataCS());
                if (!bIsReceivingInput)
                {
                    auto AdvanceStateMachine = [this, StateMachine, InDeltaSeconds]()
                    {
                        if (StateMachine->HasAnyReportedEvents())
                        {
                            PopulateReportedEvents();
                        }
#if PLATFORM_ANDROID
                        UE_LOG(LogRive, Verbose, TEXT("[%s] StateMachine->Advance"), IsInRHIThread() ? TEXT("RHIThread") : (IsInRenderingThread() ? TEXT("RenderThread") : TEXT("OtherThread")));
#endif
                        StateMachine->Advance(InDeltaSeconds);
                    };
                    if (UE::Rive::Renderer::IRiveRendererModule::RunInGameThread())
                    {
                        AdvanceStateMachine();
                    }
                    else
                    {
                        ENQUEUE_RENDER_COMMAND(DrawArtboard)(
                        [AdvanceStateMachine = MoveTemp(AdvanceStateMachine)](FRHICommandListImmediate& RHICmdList) mutable
                        {
#if PLATFORM_ANDROID
                                RHICmdList.EnqueueLambda(TEXT("StateMachine->Advance"),
                                    [AdvanceStateMachine = MoveTemp(AdvanceStateMachine)](FRHICommandListImmediate& RHICmdList)
                                {
#endif
                                    AdvanceStateMachine();
#if PLATFORM_ANDROID
                                });
#endif
                        });
                    }
                }
            }
            
            const FVector2f RiveAlignmentXY = GetRiveAlignment();
            RiveRenderTarget->DrawArtboard((uint8)RiveFitType, RiveAlignmentXY.X, RiveAlignmentXY.Y, Artboard->GetNativeArtboard(), DebugColor);
            bDrawOnceTest = true;
        }

        // Copy from render target
        // TODO. move from here
        // Separate target might be needed to let Rive draw only to separate texture
        if (GetRenderTargetToDrawOnto() != this)
        {
            FTextureRenderTargetResource* RiveFileResource = GameThread_GetRenderTargetResource();
            FTextureRenderTargetResource* RiveFileRenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();

            ENQUEUE_RENDER_COMMAND(CopyRenderTexture)(
                [this, RiveFileResource, RiveFileRenderTargetResource](FRHICommandListImmediate& RHICmdList)
                {
                    UE::Rive::Renderer::FRiveRendererUtils::CopyTextureRDG(RHICmdList, RiveFileRenderTargetResource->TextureRHI, RiveFileResource->TextureRHI);
                    // RHICmdList.CopyTexture(RenderTarget->GetResource()->TextureRHI, GetResource()->TextureRHI, FRHICopyTextureInfo());
                });
        }
        // FlushRenderingCommands();
    }
#endif // WITH_RIVE
}

UE_ENABLE_OPTIMIZATION

bool URiveFile::IsTickable() const
{
    return !HasAnyFlags(RF_ClassDefaultObject) && bIsRendering;
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
    
    UE::Rive::Renderer::IRiveRendererModule::Get().CallOrRegister_OnRendererInitialized(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &URiveFile::Initialize));
}

#if WITH_EDITOR

void URiveFile::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    // TODO. WE need custom implementation here to handle the Rive File Editor Changes
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property != nullptr)
    {
        const FName PropertyName = PropertyChangedEvent.Property->GetFName();

        if (PropertyName == GET_MEMBER_NAME_CHECKED(URiveFile, ArtboardIndex) || PropertyName == GET_MEMBER_NAME_CHECKED(URiveFile, ArtboardName))
        {
            InstanceArtboard();
        }
    }
}

#endif // WITH_EDITOR

void URiveFile::FireTrigger(const FString& InPropertyName) const
{
#if WITH_RIVE

    if (GetArtboard())
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

    if (GetArtboard())
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

float URiveFile::GetNumberValue(const FString& InPropertyName) const
{
#if WITH_RIVE

    if (GetArtboard())
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

    if (GetArtboard())
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

    if (GetArtboard())
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

    if (GetArtboard())
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

#if WITH_RIVE
    const FVector2f ArtboardSize = GetArtboard()->GetSize();
    const float TextureAspectRatio = ArtboardSize.X / ArtboardSize.Y;
    const float ViewportAspectRatio = static_cast<float>(InViewportSize.X) / InViewportSize.Y;
    
    switch (RiveFitType)
    {
        case ERiveFitType::Fill:
            NewSize = InViewportSize;
            break;
        case ERiveFitType::ScaleDown:  // Take up maximum texture size, or scale it down while containing the texture
        case ERiveFitType::Contain:
            {
                // Ensures one dimension takes up the container space fully, without clipping
                if (ViewportAspectRatio > TextureAspectRatio)
                {
                    NewSize = {
                        static_cast<int>(InViewportSize.Y * TextureAspectRatio),
                        InViewportSize.Y
                    };
                }
                else
                {
                    NewSize = {
                        InViewportSize.X,
                        static_cast<int>(InViewportSize.X / TextureAspectRatio)
                    };
                }
                // If we want to scale down and the contain size is bigger, revert to the Artboard size
                if (RiveFitType == ERiveFitType::ScaleDown)
                {
                    if (NewSize.X > SizeX || NewSize.Y > SizeY)
                    {
                        NewSize = { SizeX, SizeY };
                    }
                }
                break;
            }
        case ERiveFitType::Cover:
            {
                // This Ensures one of the dimensions will always take up container space fully, with clipping
                if (ViewportAspectRatio > TextureAspectRatio)
                {
                    NewSize = {
                        InViewportSize.X,
                        static_cast<int>(InViewportSize.X / TextureAspectRatio)
                    };
                }
                else
                {
                    NewSize = {
                        static_cast<int>(InViewportSize.Y * TextureAspectRatio),
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
                static_cast<int>(InViewportSize.Y * TextureAspectRatio),
                InViewportSize.Y
            };
            break;
        case ERiveFitType::None:
            NewSize = { SizeX, SizeY };
            break;
        default:
            NewSize = { SizeX, SizeY };
            UE_LOG(LogRive, Error, TEXT("Unknown Fit Type for Rive File"))
            break;
    }
#endif // WITH_RIVE
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

#if WITH_EDITOR

bool URiveFile::EditorImport(const FString& InRiveFilePath, TArray<uint8>& InRiveFileBuffer)
{
    if (!UE::Rive::Renderer::IRiveRendererModule::IsAvailable())
    {
        UE_LOG(LogRive, Error, TEXT("Could not load rive file as the required Rive Renderer Module is either missing or not loaded properly."));
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
        UE_LOG(LogRive, Error, TEXT("Could not load rive file as the required Rive Renderer is not initialized."));
        return false;
    }
    
#if WITH_RIVE

    rive::pls::PLSRenderContext* PLSRenderContext = RiveRenderer->GetPLSRenderContextPtr();
    if (!PLSRenderContext)
    {
        UE_LOG(LogRive, Error, TEXT("Failed to import rive file as we do not have a valid context."));
        return false;
    }
    
    RiveFilePath = InRiveFilePath;
    RiveFileData = MoveTemp(InRiveFileBuffer);
    RiveNativeFileSpan = rive::make_span(RiveFileData.GetData(), RiveFileData.Num());
    
    TUniquePtr<UE::Rive::Assets::FURAssetImporter> AssetImporter = MakeUnique<UE::Rive::Assets::FURAssetImporter>(this);

    rive::ImportResult ImportResult;
    RiveNativeFilePtr = rive::File::import(RiveNativeFileSpan, PLSRenderContext,
                                                                  &ImportResult, AssetImporter.Get());
    if (ImportResult != rive::ImportResult::success)
    {
        UE_LOG(LogRive, Error, TEXT("Failed to import rive file."));

        return false;
    }

    return true;
#endif // WITH_RIVE
}

#endif // WITH_EDITOR

void URiveFile::Initialize()
{
    if (!UE::Rive::Renderer::IRiveRendererModule::IsAvailable())
    {
        UE_LOG(LogRive, Error, TEXT("Could not load rive file as the required Rive Renderer Module is either missing or not loaded properly."));
        return;
    }

    UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();

    if (!RiveRenderer)
    {
        UE_LOG(LogRive, Error, TEXT("Failed to import rive file as we do not have a valid renderer."));
        return;
    }

    if (!RiveRenderer->IsInitialized())
    {
        UE_LOG(LogRive, Error, TEXT("Could not load rive file as the required Rive Renderer is not initialized."));
        return;
    }
    
#if WITH_RIVE

    if (ParentRiveFile && !ParentRiveFile->bIsFileImported)
    {
        ParentRiveFile->Initialize();

        // TODO: We might have to wait for the parent to finalize initializing before we can continue here.
    }
    
    if (!ParentRiveFile)
    {
        if (RiveNativeFileSpan.empty())
        {
            if (RiveFileData.IsEmpty())
            {
                UE_LOG(LogRive, Error, TEXT("Could not load an empty Rive File Data."));
                return;
            }
            RiveNativeFileSpan = rive::make_span(RiveFileData.GetData(), RiveFileData.Num());
        }
    }

    ENQUEUE_RENDER_COMMAND(URiveFileInitialize)(
    [this, RiveRenderer](FRHICommandListImmediate& RHICmdList)
    {
#if PLATFORM_ANDROID
       RHICmdList.EnqueueLambda(TEXT("URiveFile::Initialize"), [this, RiveRenderer](FRHICommandListImmediate& RHICmdList)
       {
#endif // PLATFORM_ANDROID
            if (rive::pls::PLSRenderContext* PLSRenderContext = RiveRenderer->GetPLSRenderContextPtr())
            {
                if (!ParentRiveFile)
                {
                    const TUniquePtr<UE::Rive::Assets::FURFileAssetLoader> FileAssetLoader = MakeUnique<UE::Rive::Assets::FURFileAssetLoader>(this);
                    rive::ImportResult ImportResult;

                    RiveNativeFilePtr = rive::File::import(RiveNativeFileSpan, PLSRenderContext, &ImportResult, FileAssetLoader.Get());

                    if (ImportResult != rive::ImportResult::success)
                    {
                        UE_LOG(LogRive, Error, TEXT("Failed to import rive file."));
                        return;
                    }
                }
                
                InstanceArtboard();
                PrintStats();
                bIsFileImported = true;
            }
            else
            {
                UE_LOG(LogRive, Error, TEXT("Failed to import rive file as we do not have a valid context."));
            }
#if PLATFORM_ANDROID
        });
#endif // PLATFORM_ANDROID
    });

#endif // WITH_RIVE
}

void URiveFile::InstanceArtboard()
{
    if (!UE::Rive::Renderer::IRiveRendererModule::IsAvailable())
    {
        UE_LOG(LogRive, Error, TEXT("Could not load rive file as the required Rive Renderer Module is either missing or not loaded properly."));
        return;
    }

    UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();

    if (!RiveRenderer)
    {
        UE_LOG(LogRive, Error, TEXT("Failed to import rive file as we do not have a valid renderer."));
        return;
    }

    if (!RiveRenderer->IsInitialized())
    {
        UE_LOG(LogRive, Error, TEXT("Could not load rive file as the required Rive Renderer is not initialized."));
        return;
    }

    if (!GetNativeFile())
    {
        UE_LOG(LogRive, Error, TEXT("Could not instance artboard as our native rive file is invalid."));
        return;
    }

    bIsInitialized = false;
    bIsFileImported = false;
    
    ENQUEUE_RENDER_COMMAND(URiveFileInitialize)(
    [this, RiveRenderer](FRHICommandListImmediate& RHICmdList)
    {
#if PLATFORM_ANDROID
       RHICmdList.EnqueueLambda(TEXT("URiveFile::Initialize"), [this, RiveRenderer](FRHICommandListImmediate& RHICmdList)
       {
#endif // PLATFORM_ANDROID

           RiveRenderTarget.Reset();
           RenderTarget = nullptr;
           
            if (rive::pls::PLSRenderContext* PLSRenderContext = RiveRenderer->GetPLSRenderContextPtr())
            {
                Artboard = NewObject<URiveArtboard>(this);
                if (ArtboardName.IsEmpty())
                {
                    Artboard->Initialize(GetNativeFile(), ArtboardIndex, StateMachineName);
                } else
                {
                    Artboard->Initialize(GetNativeFile(), ArtboardName, StateMachineName);
                }
                
                PrintStats();
                bIsFileImported = true;
            }
            else
            {
                UE_LOG(LogRive, Error, TEXT("Failed to import rive file as we do not have a valid context."));
            }
#if PLATFORM_ANDROID
        });
#endif // PLATFORM_ANDROID
    });
}

void URiveFile::SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass)
{
    WidgetClass = InWidgetClass;
}

UE_DISABLE_OPTIMIZATION

const URiveArtboard* URiveFile::GetArtboard() const
{
#if WITH_RIVE
    if (Artboard && Artboard->IsInitialized())
    {
        return Artboard;
    }
#endif // WITH_RIVE
    return nullptr;
}

void URiveFile::PopulateReportedEvents()
{
#if WITH_RIVE

    if (!GetArtboard()) return;
    
    if (UE::Rive::Core::FURStateMachine* StateMachine = Artboard->GetStateMachine())
    {
        const int32 NumReportedEvents = StateMachine->GetReportedEventsCount();

        TickRiveReportedEvents.Reserve(NumReportedEvents);

        for (int32 EventIndex = 0; EventIndex < NumReportedEvents; EventIndex++)
        {
            const rive::EventReport ReportedEvent = StateMachine->GetReportedEvent(EventIndex);
            if (ReportedEvent.event() != nullptr)
            {
                FRiveEvent RiveEvent;
                RiveEvent.Initialize(ReportedEvent);
                TickRiveReportedEvents.Add(MoveTemp(RiveEvent));
            }
        }

        if (!TickRiveReportedEvents.IsEmpty())
        {
            RiveEventDelegate.Broadcast(TickRiveReportedEvents.Num());
        }
    }
    else
    {
        UE_LOG(LogRive, Error, TEXT("Failed to populate reported event(s) as we could not retrieve native state machine."));
    }

#endif // WITH_RIVE
}

void URiveFile::PrintStats()
{
    rive::File* NativeFile = GetNativeFile();
    if (!NativeFile)
    {
        UE_LOG(LogRive, Error, TEXT("Could not print statistics as we have detected an empty rive file."));
        return;
    }

    FFormatNamedArguments RiveFileLoadArgs;
    RiveFileLoadArgs.Add(TEXT("Major"), FText::AsNumber(static_cast<int>(NativeFile->majorVersion)));
    RiveFileLoadArgs.Add(TEXT("Minor"), FText::AsNumber(static_cast<int>(NativeFile->minorVersion)));
    RiveFileLoadArgs.Add(TEXT("NumArtboards"), FText::AsNumber(static_cast<uint32>(NativeFile->artboardCount())));
    RiveFileLoadArgs.Add(TEXT("NumAssets"), FText::AsNumber(static_cast<uint32>(NativeFile->assets().size())));

    if (const rive::Artboard* NativeArtboard = NativeFile->artboard())
    {
        RiveFileLoadArgs.Add(TEXT("NumAnimations"), FText::AsNumber(static_cast<uint32>(NativeArtboard->animationCount())));
    }
    else
    {
        RiveFileLoadArgs.Add(TEXT("NumAnimations"), FText::AsNumber(0));
    }

    const FText RiveFileLoadMsg = FText::Format(NSLOCTEXT("FURFile", "RiveFileLoadMsg", "Using Rive Runtime : {Major}.{Minor}; Artboard(s) Count : {NumArtboards}; Asset(s) Count : {NumAssets}; Animation(s) Count : {NumAnimations}"), RiveFileLoadArgs);

    UE_LOG(LogRive, Display, TEXT("%s"), *RiveFileLoadMsg.ToString());
}

UE_ENABLE_OPTIMIZATION
