// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "Rive/RiveRenderTarget2D.h"
#include "Rive/RiveArtboard.h"
#include "Rive/RiveFile.h"

#include "IRiveRendererModule.h"
#include "RiveRenderer.h"
#include "RiveRenderTarget.h"
#include "Logs/RiveLog.h"
#include "RiveRenderer/Private/Platform/RiveRenderTargetRHI.h"

void URiveRenderTarget2D::EnforceLinearColorSpace()
{
    RenderTargetFormat = RTF_RGBA8;
    SRGB = false;
    bForceLinearGamma = true;
    bAutoGenerateMips = false;
}

URiveRenderTarget2D::URiveRenderTarget2D()
{
    Filter = TF_Bilinear;
    AddressX = TA_Clamp;
    AddressY = TA_Clamp;
    bCanCreateUAV = GRHISupportsPixelShaderUAVs;
    TargetGamma = 0.f;
    EnforceLinearColorSpace();
}

void URiveRenderTarget2D::PostLoad()
{
    Super::PostLoad();

    EnforceLinearColorSpace();

    if (IsRunningCommandlet() || HasAnyFlags(RF_ClassDefaultObject))
    {
        return;
    }

    if (!RenderTarget)
    {
        InitRiveRenderTarget2D();
    }

    Draw();
}

void URiveRenderTarget2D::DrawTestClear()
{
    // Empty draw to clear
    Draw([](rive::DrawKey,
            rive::CommandServer*,
            rive::Renderer* Renderer,
            rive::Factory* Factory) {
        auto Paint = Factory->makeRenderPaint();
        Paint->color(0xFFFFFFFF);
        rive::RawPath Path;
        Path.addRect(rive::AABB(10, 10, 40, 40));

        auto RenderPath =
            Factory->makeRenderPath(Path, rive::FillRule::nonZero);
        Renderer->drawPath(RenderPath.get(), Paint.get());
    });
}

void URiveRenderTarget2D::InitRiveRenderTarget2D()
{
    InitAutoFormat(SizeX, SizeY);

    auto Renderer = IRiveRendererModule::Get().GetRenderer();
    check(Renderer);
    RenderTarget = Renderer->CreateRenderTarget(*GetName(), this);
    RenderTarget->SetClearRenderTarget(bShouldClear);

    RenderTarget->Initialize();

    if (!IsValid(RiveDescriptor.RiveFile))
    {
        RiveArtboard = nullptr;
        UE_LOG(LogRive, Warning, TEXT("RiveDescriptor.RiveFile is invalid."));
        return;
    }

    if (!RiveDescriptor.RiveFile->ArtboardDefinitions.IsEmpty())
    {
        if (RiveDescriptor.ArtboardName.IsEmpty())
        {
            RiveDescriptor.ArtboardName =
                RiveDescriptor.RiveFile->ArtboardDefinitions[0].Name;
        }

        if (RiveDescriptor.StateMachineName.IsEmpty())
        {
            auto ArtboardDefinition =
                RiveDescriptor.RiveFile->ArtboardDefinitions.FindByPredicate(
                    [Name = RiveDescriptor.ArtboardName](auto& Itr) {
                        return Itr.Name == Name;
                    });

            // We should definitely have an FArtboardDefinition at this point.
            assert(ArtboardDefinition);

            if (!ArtboardDefinition->StateMachineNames.IsEmpty())
            {
                RiveDescriptor.StateMachineName =
                    ArtboardDefinition->StateMachineNames[0];
            }
        }
    }

    if (!RiveDescriptor.ArtboardName.IsEmpty())
    {
        auto& Builder = Renderer->GetCommandBuilder();
        RiveArtboard = RiveDescriptor.RiveFile->CreateArtboardNamed(
            Builder,
            RiveDescriptor.ArtboardName,
            RiveDescriptor.bAutoBindDefaultViewModel,
            RiveDescriptor.StateMachineName);
        UpdateArtboardSize();
        Draw();
    }
}

void URiveRenderTarget2D::Draw(DirectDrawCallback DrawCallback)
{
    auto& Builder = IRiveRendererModule::Get().GetCommandBuilder();
    Builder.Draw(RenderTarget, DrawCallback);
}

void URiveRenderTarget2D::Draw()
{
    if (RiveArtboard)
    {
        Draw(RiveArtboard, RiveDescriptor);
    }
}

void URiveRenderTarget2D::Draw(URiveArtboard* InArtboard,
                               FRiveDescriptor InDescriptor)
{
    if (!IsValid(RiveDescriptor.RiveFile) ||
        RiveDescriptor.ArtboardName.IsEmpty() || !IsValid(RiveArtboard))
        return;
    // Alignment is the full size of the render target.
    // {0,0 => SizeX,SizeY}
    FBox2f AlignmentBox{{},
                        {static_cast<float>(SizeX), static_cast<float>(SizeY)}};
    auto& Builder = IRiveRendererModule::Get().GetCommandBuilder();
    Builder.DrawArtboard(RenderTarget,
                         {InArtboard->GetNativeArtboardHandle(),
                          AlignmentBox,
                          InDescriptor.Alignment,
                          InDescriptor.FitType,
                          InDescriptor.ScaleFactor});
}
#if WITH_EDITOR
bool URiveRenderTarget2D::CanEditChange(const FProperty* InProperty) const
{
    if (!Super::CanEditChange(InProperty))
    {
        return false;
    }

    if (InProperty == nullptr)
    {
        return true;
    }

    static const TSet<FName> LockedProperties = {
        GET_MEMBER_NAME_CHECKED(UTextureRenderTarget2D, RenderTargetFormat),
        GET_MEMBER_NAME_CHECKED(UTexture, SRGB),
        GET_MEMBER_NAME_CHECKED(UTextureRenderTarget2D, bForceLinearGamma),
        GET_MEMBER_NAME_CHECKED(UTextureRenderTarget2D, bAutoGenerateMips),
    };

    return !LockedProperties.Contains(InProperty->GetFName());
}

void URiveRenderTarget2D::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent)
{
    EnforceLinearColorSpace();

    Super::PostEditChangeProperty(PropertyChangedEvent);
    const auto PropertyName = PropertyChangedEvent.GetPropertyName();

    if (PropertyName ==
        GET_MEMBER_NAME_CHECKED(URiveRenderTarget2D, bShouldClear))
    {
        RenderTarget->SetClearRenderTarget(bShouldClear);
    }
    else if (PropertyName ==
                 GET_MEMBER_NAME_CHECKED(FRiveDescriptor, RiveFile) ||
             PropertyName ==
                 GET_MEMBER_NAME_CHECKED(FRiveDescriptor, ArtboardName) ||
             PropertyName ==
                 GET_MEMBER_NAME_CHECKED(FRiveDescriptor, StateMachineName) ||
             PropertyName == GET_MEMBER_NAME_CHECKED(FRiveDescriptor,
                                                     bAutoBindDefaultViewModel))
    {
        if (!IsValid(RiveDescriptor.RiveFile))
        {
            RiveArtboard = nullptr;
            UE_LOG(LogRive, Error, TEXT("RiveDescriptor.RiveFile is invalid."));
            return;
        }

        auto& Builder = IRiveRendererModule::Get().GetCommandBuilder();
        RiveArtboard = RiveDescriptor.RiveFile->CreateArtboardNamed(
            Builder,
            RiveDescriptor.ArtboardName,
            RiveDescriptor.bAutoBindDefaultViewModel,
            RiveDescriptor.StateMachineName);
    }
    // This property was changed on the base class and thus we should recreate
    // the render target
    else
    {
        // Re create render target to match new size
        auto Renderer = IRiveRendererModule::Get().GetRenderer();
        check(Renderer);
        RenderTarget = Renderer->CreateRenderTarget(*GetName(), this);
        RenderTarget->SetClearRenderTarget(bShouldClear);
        RenderTarget->Initialize();
    }

    UpdateArtboardSize();

    if (IsValid(RiveDescriptor.RiveFile) &&
        !RiveDescriptor.ArtboardName.IsEmpty())
    {
        Draw();
    }
}
#endif

void URiveRenderTarget2D::UpdateArtboardSize()
{
    if (IsValid(RiveArtboard))
    {
        if (RiveDescriptor.FitType == ERiveFitType::Layout)
        {
            RiveArtboard->SetNativeArtboardSizeWithScale(
                SizeX,
                SizeY,
                RiveDescriptor.ScaleFactor);
        }
        else
        {
            RiveArtboard->ResetNativeArtboardSize();
        }
    }
}

uint32 URiveRenderTarget2D::CalcTextureMemorySizeEnum(
    ETextureMipCount Enum) const
{
    // Calculate size based on format.  All mips are resident on render targets
    // so we always return the same value.
    EPixelFormat Format = GetFormat();
    int32 BlockSizeX = GPixelFormats[Format].BlockSizeX;
    int32 BlockSizeY = GPixelFormats[Format].BlockSizeY;
    int32 BlockBytes = GPixelFormats[Format].BlockBytes;
    int32 NumBlocksX = (SizeX + BlockSizeX - 1) / BlockSizeX;
    int32 NumBlocksY = (SizeY + BlockSizeY - 1) / BlockSizeY;
    int32 NumBytes = NumBlocksX * NumBlocksY * BlockBytes;
    return NumBytes;
}

ETextureRenderTargetSampleCount URiveRenderTarget2D::GetSampleCount() const
{
    return ETextureRenderTargetSampleCount::RTSC_1;
}
