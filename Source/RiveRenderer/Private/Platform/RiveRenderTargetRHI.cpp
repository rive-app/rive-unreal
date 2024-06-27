#include "RiveRenderTargetRHI.h"
#include "RiveRendererRHI.h"
THIRD_PARTY_INCLUDES_START
#include "rive/pls/pls_render_target.hpp"
THIRD_PARTY_INCLUDES_END

FRiveRenderTargetRHI::FRiveRenderTargetRHI(const TSharedRef<FRiveRendererRHI>& InRiveRenderer, const FName& InRiveName, UTexture2DDynamic* InRenderTarget) :
FRiveRenderTarget(InRiveRenderer, InRiveName, InRenderTarget)
{
    
}

FRiveRenderTargetRHI::~FRiveRenderTargetRHI()
{
    
}

void FRiveRenderTargetRHI::CacheTextureTarget_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    const FTexture2DRHIRef& InRHIResource)
{
    
}

void FRiveRenderTargetRHI::Render_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    const TArray<FRiveRenderCommand>& RiveRenderCommands)
{
    
}

rive::rcp<rive::pls::PLSRenderTarget> FRiveRenderTargetRHI::
GetRenderTarget() const
{
    return nullptr;
}