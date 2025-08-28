// Fill out your copyright notice in the Description page of Project Settings.

#include "Rive/RiveRenderTarget2D.h"
#include "Rive/RiveArtboard.h"
#include "Rive/RiveFile.h"

#include "IRiveRendererModule.h"
#include "RiveRenderer.h"
#include "RiveRenderTarget.h"
#include "Logs/RiveLog.h"

TArray<FString> URiveRenderTarget2D::GetArtboardNamesForDropdown() const
{
    TArray<FString> Output;

    if (RiveDescriptor.RiveFile)
    {
        for (auto Artboard : RiveDescriptor.RiveFile->ArtboardDefinitions)
        {
            Output.Add(Artboard.Name);
        }
    }

    return Output;
}

TArray<FString> URiveRenderTarget2D::GetStateMachineNamesForDropdown() const
{
    if (RiveDescriptor.RiveFile)
    {
        for (auto Artboard : RiveDescriptor.RiveFile->ArtboardDefinitions)
        {
            if (Artboard.Name.Equals(RiveDescriptor.ArtboardName))
            {
                return Artboard.StateMachineNames;
            }
        }
    }

    return {};
}

URiveRenderTarget2D::URiveRenderTarget2D()
{
    Filter = TF_Bilinear;
    AddressX = TA_Clamp;
    AddressY = TA_Clamp;
    RenderTargetFormat = RTF_RGBA8_SRGB;
    bCanCreateUAV = GRHISupportsPixelShaderUAVs;
    bAutoGenerateMips = false;
    bForceLinearGamma = false;
    SRGB = true;
}

void URiveRenderTarget2D::PostLoad()
{
    Super::PostLoad();

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

void URiveRenderTarget2D::InitRiveRenderTarget2D()
{
    if (!IsValid(RiveDescriptor.RiveFile))
    {
        RiveArtboard = nullptr;
        UE_LOG(LogRive, Error, TEXT("RiveDescriptor.RiveFile is invalid."));
        return;
    }

    if (!RiveDescriptor.RiveFile->ArtboardDefinitions.IsEmpty())
    {
        RiveDescriptor.ArtboardName =
            RiveDescriptor.RiveFile->ArtboardDefinitions[0].Name;
        if (!RiveDescriptor.RiveFile->ArtboardDefinitions[0]
                 .StateMachineNames.IsEmpty())
            RiveDescriptor.StateMachineName =
                RiveDescriptor.RiveFile->ArtboardDefinitions[0]
                    .StateMachineNames[0];
    }

    auto Renderer = IRiveRendererModule::Get().GetRenderer();
    check(Renderer);
    RenderTarget = Renderer->CreateRenderTarget(*GetName(), this);
    RenderTarget->SetClearRenderTarget(bShouldClear);
    RenderTarget->Initialize();

    auto& Builder = Renderer->GetCommandBuilder();
    RiveArtboard = RiveDescriptor.RiveFile->CreateArtboardNamed(
        Builder,
        RiveDescriptor.ArtboardName,
        RiveDescriptor.bAutoBindDefaultViewModel);

    Draw();
}

void URiveRenderTarget2D::Draw() { Draw(RiveArtboard, RiveDescriptor); }

void URiveRenderTarget2D::Draw(URiveArtboard* InArtboard,
                               FRiveDescriptor InDescriptor)
{
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
void URiveRenderTarget2D::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent)
{
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
            RiveDescriptor.bAutoBindDefaultViewModel);
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

    Draw();
}
#endif

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
