#pragma once

THIRD_PARTY_INCLUDES_START
#undef PI
#include <rive/pls/pls_render_context_helper_impl.hpp>
#include "rive/pls/pls_image.hpp"
THIRD_PARTY_INCLUDES_END

namespace rive
{
namespace pls
{
class PLSRenderContextRHIImpl;
class PLSRenderTargetRHI : public PLSRenderTarget
{
public:
    PLSRenderTargetRHI(PLSRenderContextRHIImpl*, uint32_t width, uint32_t height);
    virtual ~PLSRenderTargetRHI() override {}
};

class PLSRenderContextRHIImpl : public PLSRenderContextHelperImpl
{
public:
    static std::unique_ptr<PLSRenderContext> MakeContext(FRHICommandListImmediate& CommandListImmediate);
    
    PLSRenderContextRHIImpl(FRHICommandListImmediate& CommandListImmediate);
    
    virtual rcp<RenderBuffer> makeRenderBuffer(RenderBufferType, RenderBufferFlags, size_t)override;
    virtual rcp<PLSTexture> makeImageTexture(uint32_t width, uint32_t height, uint32_t mipLevelCount, const uint8_t imageDataRGBA[])override;

    virtual void flush(const pls::FlushDescriptor&)override;
    
    FRHICommandListImmediate& CommandListImmediate;
};
} // namespace pls
} // namespace rive