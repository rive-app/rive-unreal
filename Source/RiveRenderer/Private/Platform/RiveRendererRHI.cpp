#include "RiveRendererRHI.h"
#include "pls_render_context_rhi_impl.hpp"
#include "RiveRenderTargetRHI.h"

TSharedPtr<IRiveRenderTarget> FRiveRendererRHI::CreateTextureTarget_GameThread(const FName& InRiveName, UTexture2DDynamic* InRenderTarget)
{
    check(IsInGameThread());

    FScopeLock Lock(&ThreadDataCS);

    const TSharedPtr<FRiveRenderTargetRHI> RiveRenderTarget = MakeShared<FRiveRenderTargetRHI>(SharedThis(this), InRiveName, InRenderTarget);

    RenderTargets.Add(InRiveName, RiveRenderTarget);

    return RiveRenderTarget;
}

DECLARE_GPU_STAT_NAMED(CreatePLSContextRHI, TEXT("CreatePLSContext_RenderThread"));
void FRiveRendererRHI::CreatePLSContext_RenderThread(FRHICommandListImmediate& RHICmdList)
{
    check(IsInRenderingThread());
    check(GDynamicRHI);
    
    FScopeLock Lock(&ThreadDataCS);

    SCOPED_GPU_STAT(RHICmdList, CreatePLSContextRHI);
    
    if(GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Null) {
        return;
    } 
    
#if WITH_RIVE
    RenderContext = rive::gpu::RenderContextRHIImpl::MakeContext(RHICmdList);
#endif // WITH_RIVE
}