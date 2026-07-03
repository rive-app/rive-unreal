/*
 * Copyright 2025 Rive
 */
#include "Ore/ore_texture_rhi.hpp"
#include "RHICommandList.h"

namespace rive::ore
{

OreTextureRHI::OreTextureRHI(const TextureDesc& desc) : lite_rtti_override(desc)
{}

void OreTextureRHI::upload(const TextureDataDesc& data)
{
    check(IsInRenderingThread());
    if (!ensure(m_texture.IsValid()) || !ensure(data.data != nullptr))
    {
        return;
    }

    // Runs during the Ore canvas render (inside the RDG-bracketed pass), so the
    // immediate command list is the one the pass is recording onto.
    FRHICommandList& RHICmdList = GRHICommandList.GetImmediateCommandList();
    const uint8* SourceData = static_cast<const uint8*>(data.data);

    if (m_texture->GetDesc().Dimension == ETextureDimension::Texture3D)
    {
        const FUpdateTextureRegion3D Region(data.x,
                                            data.y,
                                            data.z,
                                            /*SrcX*/ 0,
                                            /*SrcY*/ 0,
                                            /*SrcZ*/ 0,
                                            data.width,
                                            data.height,
                                            data.depth);
        RHICmdList.UpdateTexture3D(m_texture,
                                   data.mipLevel,
                                   Region,
                                   data.bytesPerRow,
                                   data.bytesPerRow * data.rowsPerImage,
                                   SourceData);
        return;
    }

    // 2D (and the slice-0 case of array/cube). UE's command-list update has no
    // array-slice parameter; uploading to a non-zero layer of an array/cube
    // texture isn't supported through this path yet.
    ensureMsgf(data.layer == 0,
               TEXT("OreTextureRHI::upload to array/cube layer %u is not "
                    "supported (UpdateTexture2D has no slice parameter)."),
               data.layer);

    const FUpdateTextureRegion2D Region(data.x,
                                        data.y,
                                        /*SrcX*/ 0,
                                        /*SrcY*/ 0,
                                        data.width,
                                        data.height);
    RHICmdList.UpdateTexture2D(m_texture,
                               data.mipLevel,
                               Region,
                               data.bytesPerRow,
                               SourceData);
}

OreTextureViewRHI::OreTextureViewRHI(rcp<Texture> texture,
                                     const TextureViewDesc& desc) :
    lite_rtti_override(std::move(texture), desc)
{}

} // namespace rive::ore
