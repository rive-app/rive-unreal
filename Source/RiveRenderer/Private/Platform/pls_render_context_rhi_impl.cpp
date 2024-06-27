#include "pls_render_context_rhi_impl.hpp"
THIRD_PARTY_INCLUDES_START
#include "rive/pls/pls_image.hpp"
THIRD_PARTY_INCLUDES_END

namespace rive
{
namespace pls
{

class RenderBufferRHIImpl : public lite_rtti_override<RenderBuffer, RenderBufferRHIImpl>
{
    RenderBufferRHIImpl(FRHICommandListBase& commandList, RenderBufferType type,
    RenderBufferFlags flags, size_t sizeInBytes) : lite_rtti_override(type, flags, sizeInBytes), commandList(commandList)
    {
        switch(type)
        {
            case RenderBufferType::vertex:
            {
                FRHIResourceCreateInfo Info(TEXT("RenderBufferRHIImpl_"));
                m_buffer = commandList.CreateVertexBuffer(sizeInBytes,
                    EBufferUsageFlags::Dynamic, ERHIAccess::WriteOnlyMask, Info);
                break;
            }{}
            case RenderBufferType::index:
                FRHIResourceCreateInfo Info(TEXT("RenderBufferRHIImpl_"));
                m_buffer = commandList.CreateIndexBuffer(sizeof(uint16_t), sizeInBytes,
                    EBufferUsageFlags::Dynamic, ERHIAccess::WriteOnlyMask, Info);
                break;
            default:
                break;
        }
        
        
    }
    
    virtual void* onMap()override
    {
        return commandList.LockBuffer(m_buffer, 0, sizeInBytes(), EResourceLockMode::RLM_WriteOnly_NoOverwrite);
    }
    
    virtual void onUnmap()override
    {
        commandList.UnlockBuffer(m_buffer);
    }

private:
    FBufferRHIRef m_buffer;
    FRHICommandListBase& commandList;
};

class PLSTextureRHIImpl : public PLSTexture
{
public:
    PLSTextureRHIImpl(PLSRenderContextRHIImpl* context, uint32_t width, uint32_t height, uint32_t mipLevelCount, const uint8_t imageDataRGBA[]) : 
        PLSTexture(width, height)
    {
        auto Desc = FRHITextureCreateDesc::Create2D(TEXT("PLSTextureRHIImpl_"), width, height, EPixelFormat::PF_R8G8B8A8_UINT);
        Desc.SetNumMips(mipLevelCount);
        _texture = GDynamicRHI->RHICreateTexture(context->CommandListImmediate, Desc);
        GDynamicRHI->RHIUpdateTexture2D(context->CommandListImmediate, _texture, 0,
            FUpdateTextureRegion2D(0, 0, 0, 0, width, height), 0, imageDataRGBA);
    }
private:
    FTextureRHIRef  _texture;
};

PLSRenderTargetRHI::PLSRenderTargetRHI(PLSRenderContextRHIImpl* context, uint32_t width, uint32_t height) :
PLSRenderTarget(width, height)
{
    
}

std::unique_ptr<PLSRenderContext> PLSRenderContextRHIImpl::MakeContext(FRHICommandListImmediate& CommandListImmediate)
{
    auto plsContextImpl = std::make_unique<PLSRenderContextRHIImpl>(CommandListImmediate);
    return std::make_unique<PLSRenderContext>(std::move(plsContextImpl));
}

PLSRenderContextRHIImpl::PLSRenderContextRHIImpl(FRHICommandListImmediate& CommandListImmediate): CommandListImmediate(CommandListImmediate)
{
    m_platformFeatures.supportsRasterOrdering = false;
}

rive::rcp<rive::RenderBuffer> PLSRenderContextRHIImpl::makeRenderBuffer(RenderBufferType type,
    RenderBufferFlags flags, size_t sizeInBytes)
{
    return make_rcp<RenderBufferRHIImpl>(CommandListImmediate, type, flags, sizeInBytes);
}

rcp<PLSTexture> PLSRenderContextRHIImpl::makeImageTexture(uint32_t width,
    uint32_t height,
    uint32_t mipLevelCount,
    const uint8_t imageDataRGBA[])
{
    // We assume we are in the render thread, which is not always
    // the same thread throughout the lifetime  of a process
    // check(IsInRenderingThread());
    return make_rcp<PLSTextureRHIImpl>(this, width, height, mipLevelCount, imageDataRGBA);
}

void PLSRenderContextRHIImpl::flush(const pls::FlushDescriptor&)
{

}

}
}