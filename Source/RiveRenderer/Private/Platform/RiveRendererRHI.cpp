// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "RiveRendererRHI.h"
#include "RenderContextRHIImpl.hpp"
#include "RiveRenderTargetRHI.h"
#include "RHICommandList.h"
#include "RenderGraphUtils.h"
#include "RenderGraphResources.h"

static const FString RiveRenderOverrideDescription =
    TEXT("Forces a specific rendering interlock mode for rive renderer.\n"
         "\tatomics: Forces atomic interlock mode\n"
         "\traster: Forces raster ordered interlock mode\n"
         "\tmsaa: Forces msaa interlock mode\n");

FRiveRendererRHI::FRiveRendererRHI()
{
    FCommandLine::RegisterArgument(TEXT("riveRenderOverride"),
                                   ECommandLineArgumentFlags::GameContexts |
                                       ECommandLineArgumentFlags::EditorContext,
                                   RiveRenderOverrideDescription);
}

TSharedPtr<FRiveRenderTarget> FRiveRendererRHI::CreateRenderTarget(
    const FName& InRiveName,
    UTexture2DDynamic* InRenderTarget)
{
    check(IsInGameThread());

    const TSharedPtr<FRiveRenderTargetRHI> RiveRenderTarget =
        MakeShared<FRiveRenderTargetRHI>(this, InRiveName, InRenderTarget);

    return RiveRenderTarget;
}

TSharedPtr<FRiveRenderTarget> FRiveRendererRHI::CreateRenderTarget(
    const FName& InRiveName,
    UTextureRenderTarget2D* InRenderTarget)
{
    check(IsInGameThread());

    const TSharedPtr<FRiveRenderTargetRHI> RiveRenderTarget =
        MakeShared<FRiveRenderTargetRHI>(this, InRiveName, InRenderTarget);

    return RiveRenderTarget;
}

TSharedPtr<FRiveRenderTarget> FRiveRendererRHI::CreateRenderTarget(
    FRDGBuilder& GraphBuilder,
    const FName& InRiveName,
    FRDGTextureRef InRenderTarget)
{
    return MakeShared<FRiveRenderTargetRHI>(GraphBuilder,
                                            this,
                                            InRiveName,
                                            InRenderTarget);
    ;
}

TSharedPtr<FRiveRenderTarget> FRiveRendererRHI::CreateRenderTarget(
    const FName& InRiveName,
    FRenderTarget* RenderTarget)
{
    return MakeShared<FRiveRenderTargetRHI>(this, InRiveName, RenderTarget);
}

DECLARE_GPU_STAT_NAMED(CreateRenderContext,
                       TEXT("FRiveRendererRHI::CreateRenderContext"));
void FRiveRendererRHI::CreateRenderContext(FRHICommandListImmediate& RHICmdList)
{
    check(IsInRenderingThread());
    check(GDynamicRHI);

    SCOPED_GPU_STAT(RHICmdList, CreateRenderContext);

    if (GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Null)
    {
        return;
    }

    RenderContextRHIImpl::RHICapabilitiesOverrides Overrides;

    FString renderOverrides;
    if (FParse::Value(FCommandLine::Get(),
                      TEXT("riveRenderOverride="),
                      renderOverrides))
    {
        auto lower = renderOverrides.ToLower();
        if (lower == TEXT("atomics"))
        {
            Overrides.bSupportsPixelShaderUAVs = true;
            Overrides.bSupportsRasterOrderViews = false;
        }
        else if (lower == TEXT("raster"))
        {
            Overrides.bSupportsPixelShaderUAVs = false;
            Overrides.bSupportsRasterOrderViews = true;
        }
        else if (lower == TEXT("msaa"))
        {
            Overrides.bSupportsPixelShaderUAVs = false;
            Overrides.bSupportsRasterOrderViews = false;
        }
    }

    RenderContext = RenderContextRHIImpl::MakeContext(RHICmdList, Overrides);
}
