#pragma once
// rive's math headers declare `math::PI`, which collides with the Windows `PI`
// macro that a prior unity-build file may have pulled in. Undef it before the
// rive include (CoreMinimal.h re-defines it afterwards). No THIRD_PARTY
// wrappers here — this is included before any UE header, so that macro isn't
// defined yet.
#undef PI
#include <rive/renderer/ore/ore_types.hpp>

#include "CoreMinimal.h"
#include "PixelFormat.h"
#include "RHI.h"
#include "RHIDefinitions.h"
#include "RiveCommandBuilder.h"
#include "RiveOrderShaderHandler.h"
#include "Ore/OrePlatformRHI.h"
#include "Logs/RiveRendererLog.h"

FORCEINLINE_DEBUGGABLE rive::ore::TextureType OreTextureTypeFromDimension(
    ETextureDimension dimension)
{
    switch (dimension)
    {
        case ETextureDimension::Texture2D:
            return rive::ore::TextureType::texture2D;
        case ETextureDimension::Texture2DArray:
            return rive::ore::TextureType::array2D;
        case ETextureDimension::Texture3D:
            return rive::ore::TextureType::texture3D;
        case ETextureDimension::TextureCube:
        case ETextureDimension::TextureCubeArray:
            return rive::ore::TextureType::cube;
        default:
            ensureMsgf(false,
                       TEXT("Unsupported ETextureDimension for Rive ORE: %d"),
                       static_cast<int32>(dimension));
            return rive::ore::TextureType::texture2D;
    }
}

// The SRV's target/dimension is taken from the *view* desc, not the underlying
// texture. A view of a single slice of an array (or a single face of a cube) is
// a 2D view over a 2D-array/cube resource; deriving the dimension from the
// texture instead (SetDimensionFromTexture) produces a view whose target does
// not match what the shader binds. Most RHIs tolerate the mismatch, but some
// reject it when the view is created. This mirrors the reference D3D12 backend,
// which builds the SRV dimension straight from desc.dimension.
FORCEINLINE_DEBUGGABLE ETextureDimension
ETextureDimensionFromOreViewDesc(const rive::ore::TextureViewDesc& desc)
{
    switch (desc.dimension)
    {
        case rive::ore::TextureViewDimension::texture2D:
            return desc.layerCount > 1 ? ETextureDimension::Texture2DArray
                                       : ETextureDimension::Texture2D;
        case rive::ore::TextureViewDimension::array2D:
            return ETextureDimension::Texture2DArray;
        case rive::ore::TextureViewDimension::cube:
            return ETextureDimension::TextureCube;
        case rive::ore::TextureViewDimension::cubeArray:
            return ETextureDimension::TextureCubeArray;
        case rive::ore::TextureViewDimension::texture3D:
            return ETextureDimension::Texture3D;
    }

    ensureMsgf(false,
               TEXT("Unsupported Rive ORE texture view dimension: %d"),
               static_cast<int32>(desc.dimension));
    return ETextureDimension::Texture2D;
}

FORCEINLINE_DEBUGGABLE EPixelFormat
PixelFormatFromOreFormat(rive::ore::TextureFormat format)
{
    using rive::ore::TextureFormat;

    switch (format)
    {
        case TextureFormat::rgba8unorm:
            return PF_R8G8B8A8;

        case TextureFormat::bgra8unorm:
            return PF_B8G8R8A8;

        case TextureFormat::r8unorm:
            return PF_R8;

        case TextureFormat::rg8unorm:
            return PF_R8G8;

        case TextureFormat::r16float:
            return PF_R16F;

        case TextureFormat::rg16float:
            return PF_G16R16F;

        case TextureFormat::rgba16float:
            return PF_FloatRGBA;

        case TextureFormat::r32float:
            return PF_R32_FLOAT;

        case TextureFormat::rg32float:
            return PF_G32R32F;

        case TextureFormat::rgba32float:
            return PF_A32B32G32R32F;

        case TextureFormat::rgba8snorm:
            return PF_R8G8B8A8_SNORM;

        case TextureFormat::rgb10a2unorm:
            return PF_A2B10G10R10;

        case TextureFormat::r11g11b10float:
            return PF_FloatR11G11B10;

        case TextureFormat::bc1unorm:
            return PF_DXT1;
        case TextureFormat::bc3unorm:
            return PF_DXT5;
        case TextureFormat::bc7unorm:
            return PF_BC7;
        case TextureFormat::etc2rgb8:
            return PF_ETC2_RGB;
        case TextureFormat::etc2rgba8:
            return PF_ETC2_RGBA;
        case TextureFormat::astc4x4:
            return PF_ASTC_4x4;
        case TextureFormat::astc6x6:
            return PF_ASTC_6x6;
        case TextureFormat::astc8x8:
            return PF_ASTC_8x8;

        case TextureFormat::depth32floatStencil8:
        case TextureFormat::depth24plusStencil8:
            return PF_DepthStencil;

        case TextureFormat::depth16unorm:
        case TextureFormat::depth32float:
            // PF_ShadowDepth is a single-plane 16-bit depth (R16_TYPELESS), the
            // only depth-targetable UE format with no stencil plane. There is a
            // precision mismatch for depth32float: the standalone backends use
            // a true 32-bit float depth (D32_FLOAT), but UE exposes NO
            // single-plane 32-bit float depth format. The only 32-bit option is
            // PF_DepthStencil (R32G8X24 — D32_FLOAT_S8X24, 32-bit float depth
            // by default since r.D3D12.Depth24Bit=0), which drags along an
            // unused 8-bit stencil plane and costs 8 bytes/px instead of 2.
            // 16-bit renders shadows correctly and is standard for shadow maps,
            // so it is the default. On RHIs where the single-plane path is not
            // a robust depth-render-target (a 2D shadow map renders no usable
            // depth), GRHIOreNeedsDepthStencilShadowFormat selects the
            // two-plane PF_DepthStencil instead; makeTextureView's depth-plane
            // handling already supports either format. See OrePlatformRHI.h.
            return GRHIOreNeedsDepthStencilShadowFormat ? PF_DepthStencil
                                                        : PF_ShadowDepth;

        default:
            ensureMsgf(false,
                       TEXT("Unsupported Rive ORE texture format: %d"),
                       static_cast<int32>(format));
            return PF_Unknown;
    }
}

FORCEINLINE_DEBUGGABLE ESamplerAddressMode
SamplerAddressModeFromOre(rive::ore::WrapMode mode)
{
    switch (mode)
    {
        case rive::ore::WrapMode::clampToEdge:
            return AM_Clamp;

        case rive::ore::WrapMode::repeat:
            return AM_Wrap;

        case rive::ore::WrapMode::mirrorRepeat:
            return AM_Mirror;

        default:
            ensureMsgf(false,
                       TEXT("Unsupported Rive ORE sampler address mode: %d"),
                       static_cast<int32>(mode));
            return AM_Clamp;
    }
}

FORCEINLINE_DEBUGGABLE ESamplerFilter
SamplerFilterFromOre(const rive::ore::SamplerDesc& desc)
{
    const bool bMinLinear = desc.minFilter == rive::ore::Filter::linear;
    const bool bMagLinear = desc.magFilter == rive::ore::Filter::linear;
    const bool bMipLinear = desc.mipmapFilter == rive::ore::Filter::linear;
    const bool bAnisotropic = desc.maxAnisotropy > 1;

    if (bAnisotropic)
    {
        return bMipLinear ? SF_AnisotropicLinear : SF_AnisotropicPoint;
    }

    if (!bMinLinear && !bMagLinear)
    {
        return SF_Point;
    }

    return bMipLinear ? SF_Trilinear : SF_Bilinear;
}

FORCEINLINE_DEBUGGABLE FSamplerStateInitializerRHI
SamplerStateInitializerFromOreDesc(const rive::ore::SamplerDesc& desc)
{
    FSamplerStateInitializerRHI Initializer;

    Initializer.Filter = SamplerFilterFromOre(desc);
    Initializer.AddressU = SamplerAddressModeFromOre(desc.wrapU);
    Initializer.AddressV = SamplerAddressModeFromOre(desc.wrapV);
    Initializer.AddressW = SamplerAddressModeFromOre(desc.wrapW);

    Initializer.MipBias = 0.0f;
    Initializer.MinMipLevel = desc.minLod;
    Initializer.MaxMipLevel = desc.maxLod;
    Initializer.MaxAnisotropy = desc.maxAnisotropy;

    // Shadow / comparison samplers: Rive sets `compare` for SampleCmp-based
    // PCF. Without this the sampler is built as a regular (non-comparison)
    // sampler and the shader's depth comparison always passes -> shadows
    // disappear. UE's RHI only exposes a Less comparison (SCF_Less) versus none
    // (SCF_Never).
    Initializer.SamplerComparisonFunction =
        desc.compare == rive::ore::CompareFunction::none ? SCF_Never : SCF_Less;

    return Initializer;
}

FORCEINLINE_DEBUGGABLE ECompareFunction
ToRhiCompareFunction(rive::ore::CompareFunction compare)
{
    switch (compare)
    {
        case rive::ore::CompareFunction::never:
            return CF_Never;
        case rive::ore::CompareFunction::less:
            return CF_Less;
        case rive::ore::CompareFunction::lessEqual:
            return CF_LessEqual;
        case rive::ore::CompareFunction::greater:
            return CF_Greater;
        case rive::ore::CompareFunction::greaterEqual:
            return CF_GreaterEqual;
        case rive::ore::CompareFunction::equal:
            return CF_Equal;
        case rive::ore::CompareFunction::notEqual:
            return CF_NotEqual;
        case rive::ore::CompareFunction::always:
        case rive::ore::CompareFunction::none:
            return CF_Always;
    }

    return CF_Always;
}

FORCEINLINE_DEBUGGABLE EBlendFactor
ToRhiBlendFactor(rive::ore::BlendFactor factor)
{
    switch (factor)
    {
        case rive::ore::BlendFactor::zero:
            return BF_Zero;

        case rive::ore::BlendFactor::one:
            return BF_One;

        case rive::ore::BlendFactor::srcAlpha:
            return BF_SourceAlpha;

        case rive::ore::BlendFactor::oneMinusSrcAlpha:
            return BF_InverseSourceAlpha;

        case rive::ore::BlendFactor::dstAlpha:
            return BF_DestAlpha;

        case rive::ore::BlendFactor::oneMinusDstAlpha:
            return BF_InverseDestAlpha;

        case rive::ore::BlendFactor::srcColor:
            return BF_SourceColor;

        case rive::ore::BlendFactor::oneMinusSrcColor:
            return BF_InverseSourceColor;

        case rive::ore::BlendFactor::dstColor:
            return BF_DestColor;

        case rive::ore::BlendFactor::oneMinusDstColor:
            return BF_InverseDestColor;

        case rive::ore::BlendFactor::srcAlphaSaturated:
            return BF_SourceAlpha;

        case rive::ore::BlendFactor::blendColor:
            return BF_SourceColor;

        case rive::ore::BlendFactor::oneMinusBlendColor:
            return BF_InverseSourceColor;
    }

    return BF_One;
}

FORCEINLINE_DEBUGGABLE EBlendOperation ToRhiBlendOp(rive::ore::BlendOp op)
{
    switch (op)
    {
        case rive::ore::BlendOp::add:
            return BO_Add;

        case rive::ore::BlendOp::subtract:
            return BO_Subtract;

        case rive::ore::BlendOp::reverseSubtract:
            return BO_ReverseSubtract;

        case rive::ore::BlendOp::min:
            return BO_Min;

        case rive::ore::BlendOp::max:
            return BO_Max;
    }

    return BO_Add;
}

FORCEINLINE_DEBUGGABLE ERasterizerCullMode
ToRhiCullMode(rive::ore::CullMode CM, bool bFrontIsCCW = true)
{
    switch (CM)
    {
        case rive::ore::CullMode::none:
            return CM_None;
        case rive::ore::CullMode::front:
            return bFrontIsCCW ? CM_CCW : CM_CW;
        case rive::ore::CullMode::back:
            return bFrontIsCCW ? CM_CW : CM_CCW;
    }

    return CM_None;
}

FORCEINLINE_DEBUGGABLE EStencilOp ToRHIStencilOp(rive::ore::StencilOp Op)
{
    switch (Op)
    {
        case rive::ore::StencilOp::keep:
            return SO_Keep;

        case rive::ore::StencilOp::zero:
            return SO_Zero;

        case rive::ore::StencilOp::replace:
            return SO_Replace;

        case rive::ore::StencilOp::incrementClamp:
            return SO_SaturatedIncrement;

        case rive::ore::StencilOp::decrementClamp:
            return SO_SaturatedDecrement;

        case rive::ore::StencilOp::invert:
            return SO_Invert;

        case rive::ore::StencilOp::incrementWrap:
            return SO_Increment;

        case rive::ore::StencilOp::decrementWrap:
            return SO_Decrement;
    }

    return SO_Keep;
}

FORCEINLINE_DEBUGGABLE EVertexElementType
VertexFormatToVertexElement(rive::ore::VertexFormat Format)
{
    switch (Format)
    {
        case rive::ore::VertexFormat::float1:
            return VET_Float1;
        case rive::ore::VertexFormat::float2:
            return VET_Float2;
        case rive::ore::VertexFormat::float3:
            return VET_Float3;
        case rive::ore::VertexFormat::float4:
            return VET_Float4;
        case rive::ore::VertexFormat::float16x2:
            return VET_Half2;
        case rive::ore::VertexFormat::float16x4:
            return VET_Half4;
        case rive::ore::VertexFormat::uint8x4:
            return VET_UByte4;
        case rive::ore::VertexFormat::unorm8x4:
            return VET_UByte4N;
        case rive::ore::VertexFormat::snorm8x4:
            return VET_PackedNormal;
        case rive::ore::VertexFormat::uint16x2:
            return VET_UShort2;
        case rive::ore::VertexFormat::uint16x4:
            return VET_UShort4;
        case rive::ore::VertexFormat::unorm16x2:
            return VET_UShort2N;
        case rive::ore::VertexFormat::sint8x4:
            return VET_UByte4;
        case rive::ore::VertexFormat::sint16x2:
            return VET_Short2;
        case rive::ore::VertexFormat::sint16x4:
            return VET_Short4;
        case rive::ore::VertexFormat::snorm16x2:
            return VET_Short2N;
        case rive::ore::VertexFormat::uint32:
            return VET_UInt;
    }

    checkNoEntry();
    return VET_None;
}

FORCEINLINE_DEBUGGABLE ERenderTargetLoadAction
RenderTargetLoadActionFromLoadOp(rive::ore::LoadOp LoadOp)
{
    switch (LoadOp)
    {
        case rive::ore::LoadOp::clear:
            return ERenderTargetLoadAction::EClear;
        case rive::ore::LoadOp::load:
            return ERenderTargetLoadAction::ELoad;
        case rive::ore::LoadOp::dontCare:
            return ERenderTargetLoadAction::ENoAction;
    }

    checkNoEntry();
    return ERenderTargetLoadAction::ENoAction;
}

FORCEINLINE_DEBUGGABLE ERenderTargetStoreAction
RenderTargetStoreActionFromStoreOp(rive::ore::StoreOp StoreOp,
                                   bool bHasResolveTarget)
{
    switch (StoreOp)
    {
        case rive::ore::StoreOp::discard:
            return ERenderTargetStoreAction::ENoAction;
        case rive::ore::StoreOp::store:
            if (bHasResolveTarget)
                return ERenderTargetStoreAction::EMultisampleResolve;
            return ERenderTargetStoreAction::EStore;
    }

    checkNoEntry();
    return ERenderTargetStoreAction::ENoAction;
}

FORCEINLINE_DEBUGGABLE rive::ore::TextureFormat OreFormatFromPixelFormat(
    EPixelFormat format)
{
    using rive::ore::TextureFormat;

    switch (format)
    {
        case PF_R8G8B8A8:
            return TextureFormat::rgba8unorm;
        case PF_R8G8B8A8_SNORM:
            return TextureFormat::rgba8snorm;
        case PF_B8G8R8A8:
            return TextureFormat::bgra8unorm;
        case PF_R8:
            return TextureFormat::r8unorm;
        case PF_R8G8:
            return TextureFormat::rg8unorm;
        case PF_R16F:
            return TextureFormat::r16float;
        case PF_G16R16F:
            return TextureFormat::rg16float;
        case PF_FloatRGBA:
            return TextureFormat::rgba16float;
        case PF_R32_FLOAT:
            return TextureFormat::r32float;
        case PF_G32R32F:
            return TextureFormat::rg32float;
        case PF_A32B32G32R32F:
            return TextureFormat::rgba32float;
        case PF_A2B10G10R10:
            return TextureFormat::rgb10a2unorm;
        case PF_FloatR11G11B10:
            return TextureFormat::r11g11b10float;
        case PF_DepthStencil:
            return TextureFormat::depth32floatStencil8;
        case PF_ShadowDepth:
            return TextureFormat::depth32float;
        case PF_DXT1:
            return TextureFormat::bc1unorm;
        case PF_DXT5:
            return TextureFormat::bc3unorm;
        case PF_BC7:
            return TextureFormat::bc7unorm;
        case PF_ETC2_RGB:
            return TextureFormat::etc2rgb8;
        case PF_ETC2_RGBA:
            return TextureFormat::etc2rgba8;
        case PF_ASTC_4x4:
            return TextureFormat::astc4x4;
        case PF_ASTC_6x6:
            return TextureFormat::astc6x6;
        case PF_ASTC_8x8:
            return TextureFormat::astc8x8;
        default:
            ensureMsgf(false,
                       TEXT("Unsupported EPixelFormat for Rive ORE: %d"),
                       static_cast<int32>(format));
            return TextureFormat::rgba8unorm;
    }
}

FORCEINLINE_DEBUGGABLE bool FormatIsDepthOrStencil(
    rive::ore::TextureFormat Format)
{
    return Format == rive::ore::TextureFormat::depth16unorm ||
           Format == rive::ore::TextureFormat::depth24plusStencil8 ||
           Format == rive::ore::TextureFormat::depth32float ||
           Format == rive::ore::TextureFormat::depth32floatStencil8;
}
