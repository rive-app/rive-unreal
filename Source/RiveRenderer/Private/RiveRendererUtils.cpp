// Copyright Rive, Inc. All rights reserved.

#include "RiveRendererUtils.h"

#include "Engine/TextureRenderTarget2D.h"
#include "MediaShaders.h"
#include "RenderGraphBuilder.h"
#include "ScreenPass.h"
#include "UObject/Package.h"

UTextureRenderTarget2D* FRiveRendererUtils::CreateDefaultRenderTarget(
    FIntPoint InTargetSize,
    EPixelFormat PixelFormat,
    bool bCanCreateUAV)
{
    UTextureRenderTarget2D* const RenderTarget =
        NewObject<UTextureRenderTarget2D>(GetTransientPackage());

    RenderTarget->bForceLinearGamma = false;

    RenderTarget->bAutoGenerateMips = false;

    RenderTarget->OverrideFormat = PixelFormat;

    RenderTarget->bCanCreateUAV = bCanCreateUAV;

    RenderTarget->ResizeTarget(InTargetSize.X, InTargetSize.Y);

    return RenderTarget;
}

FIntPoint FRiveRendererUtils::GetRenderTargetSize(
    const UTextureRenderTarget2D* InRenderTarget)
{
    check(IsValid(InRenderTarget));

    return FIntPoint(InRenderTarget->SizeX, InRenderTarget->SizeY);
}

void FRiveRendererUtils::CopyTextureRDG(FRHICommandListImmediate& RHICmdList,
                                        FTextureRHIRef SourceTexture,
                                        FTextureRHIRef DestTexture)
{
    FRDGBuilder GraphBuilder(RHICmdList);

    // Register an external RDG rexture from the source texture
    FRDGTexture* InputTexture = GraphBuilder.RegisterExternalTexture(
        CreateRenderTarget(SourceTexture,
                           TEXT("PixelCaptureCopySourceTexture")));

    // Register an external RDG texture from the output buffer
    FRDGTexture* OutputTexture = GraphBuilder.RegisterExternalTexture(
        CreateRenderTarget(DestTexture, TEXT("PixelCaptureCopyDestTexture")));

    if (InputTexture->Desc.Format == OutputTexture->Desc.Format &&
        InputTexture->Desc.Extent.X == OutputTexture->Desc.Extent.X &&
        InputTexture->Desc.Extent.Y == OutputTexture->Desc.Extent.Y)
    {
        // The formats are the same and size are the same. simple copy
        AddDrawTexturePass(GraphBuilder,
                           GetGlobalShaderMap(GMaxRHIFeatureLevel),
                           InputTexture,
                           OutputTexture,
                           FRDGDrawTextureInfo());
    }
    else
    {
        // TODO (matthew.cotton) A lot of this was copied from AddScreenPass and
        // altered for our use. This was altered because the FViewInfo we create
        // here on the stack is added to the RHI queue and is accessed possibly
        // after this stack frame was released. Originally we tried just
        // changing it to static so we could reuse it, but the object does not
        // allow copying. So the current implementation creates the View on the
        // heap using a shared ptr that is then captured in a lambda for the RHI
        // queue. This allows us to update the view on calls to this function
        // and not worry about it's lifetime. The todo is here because there has
        // to be a better way to achieve what we want without all of this song
        // and dance.

        // FIntRect ViewportRect(FIntPoint(150, 150),
        // InputTexture->Desc.Extent);

        // The formats or size differ to pixel shader stuff
        // Configure source/output viewport to get the right UV scaling from
        // source texture to output texture
        FScreenPassTextureViewport InputViewport(InputTexture);

        FScreenPassTextureViewport OutputViewport(OutputTexture);

        FGlobalShaderMap* GlobalShaderMap =
            GetGlobalShaderMap(GMaxRHIFeatureLevel);

        TShaderMapRef<FScreenPassVS> VertexShader(GlobalShaderMap);

        // In cases where texture is converted from a format that doesn't have A
        // channel, we want to force set it to 1.
        int32 ConversionOperation = 0; // None

        FModifyAlphaSwizzleRgbaPS::FPermutationDomain PermutationVector;

        PermutationVector.Set<FModifyAlphaSwizzleRgbaPS::FConversionOp>(
            ConversionOperation);

        // Rectangle area to use from source
        const FIntRect ViewRect(FIntPoint::ZeroValue,
                                InputTexture->Desc.Extent);

        TShaderMapRef<FModifyAlphaSwizzleRgbaPS> PixelShader(GlobalShaderMap,
                                                             PermutationVector);

        FModifyAlphaSwizzleRgbaPS::FParameters* PixelShaderParameters =
            PixelShader->AllocateAndSetParameters(GraphBuilder,
                                                  InputTexture,
                                                  OutputTexture);

        FRHIBlendState* BlendState =
            FScreenPassPipelineState::FDefaultBlendState::GetRHI();

        FRHIDepthStencilState* DepthStencilState =
            FScreenPassPipelineState::FDefaultDepthStencilState::GetRHI();

        AddDrawScreenPass(GraphBuilder,
                          RDG_EVENT_NAME("PixelCapturerSwizzle"),
                          FScreenPassViewInfo(),
                          OutputViewport,
                          InputViewport,
                          VertexShader,
                          PixelShader,
                          PixelShaderParameters);
    }

    GraphBuilder.Execute();
}
