// Copyright Rive, Inc. All rights reserved.

#include "RiveRendererUtils.h"

#include "Engine/TextureRenderTarget2D.h"
#include "MediaShaders.h"
#include "UObject/Package.h"

UTextureRenderTarget2D* UE::Rive::Renderer::FRiveRendererUtils::CreateDefaultRenderTarget(FIntPoint InTargetSize, EPixelFormat PixelFormat, bool bCanCreateUAV)
{
    UTextureRenderTarget2D* const RenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage());

    RenderTarget->bForceLinearGamma = false;

    RenderTarget->bAutoGenerateMips = false;

    RenderTarget->OverrideFormat = PixelFormat;

    RenderTarget->bCanCreateUAV = bCanCreateUAV;
    
    RenderTarget->ResizeTarget(InTargetSize.X, InTargetSize.Y);

    return RenderTarget;
}

FIntPoint UE::Rive::Renderer::FRiveRendererUtils::GetRenderTargetSize(const UTextureRenderTarget2D* InRenderTarget)
{
    check(IsValid(InRenderTarget));

    return FIntPoint(InRenderTarget->SizeX, InRenderTarget->SizeY);
}

void UE::Rive::Renderer::FRiveRendererUtils::CopyTextureRDG(FRHICommandListImmediate& RHICmdList, FTextureRHIRef SourceTexture, FTextureRHIRef DestTexture)
{
    FRHITexture2D* SourceTexture2D = (FRHITexture2D*)SourceTexture.GetReference();
    check(SourceTexture2D);

    FRHITexture2D* DestTexture2D = (FRHITexture2D*)DestTexture.GetReference();
    check(DestTexture2D);

    if (SourceTexture2D->GetFormat() == DestTexture2D->GetFormat()
        && SourceTexture2D->GetSizeX() == DestTexture2D->GetSizeX()
        && SourceTexture2D->GetSizeY() == DestTexture2D->GetSizeY())
    {
        // The formats are the same and size are the same. simple copy
        RHICmdList.CopyTexture(SourceTexture2D, DestTexture2D, FRHICopyTextureInfo());
    }
    else
    {
        // The formats or size differ to pixel shader stuff
        // FModifyAlphaSwizzleRgbaPS is handled differently here than in UE5. We follow a similar setup to UMediaCapture::Capture_RenderThread.

        RHICmdList.Transition(FRHITransitionInfo(DestTexture2D, ERHIAccess::Unknown, ERHIAccess::CopySrc));

        FRHIRenderPassInfo RPInfo(DestTexture2D, ERenderTargetActions::DontLoad_Store);
        RHICmdList.BeginRenderPass(RPInfo, TEXT("RiveCopyTextureRDG"));

        FGraphicsPipelineStateInitializer GraphicsPSOInit;
        RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

        // Configure vertex shader
        FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
        TShaderMapRef<FMediaShadersVS> VertexShader(GlobalShaderMap);

        // Configure pixel shader
        // In cases where texture is converted from a format that doesn't have A channel, we want to force set it to 1.
        FModifyAlphaSwizzleRgbaPS::FPermutationDomain PermutationVector;
        const int32 ConversionOperation = 0; // None
        PermutationVector.Set<FModifyAlphaSwizzleRgbaPS::FConversionOp>(ConversionOperation);

        TShaderMapRef<FModifyAlphaSwizzleRgbaPS> PixelShader(GlobalShaderMap, PermutationVector);

        // Configure graphics pipeline
        GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
        GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
        GraphicsPSOInit.BlendState = TStaticBlendStateWriteMask<CW_RGBA, CW_NONE, CW_NONE, CW_NONE, CW_NONE, CW_NONE, CW_NONE, CW_NONE>::GetRHI();
        GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GMediaVertexDeclaration.VertexDeclarationRHI;
        GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
        GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

        SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
        PixelShader->SetParameters(RHICmdList, SourceTexture2D);

        // Draw full size quad into render target
        FVertexBufferRHIRef VertexBuffer = CreateTempMediaVertexBuffer(0.f, 1.f, 0.f, 1.f);
        RHICmdList.SetStreamSource(0, VertexBuffer, 0);

        // Set viewport to texture size
        const FIntVector TextureSize = SourceTexture2D->GetSizeXYZ();

        RHICmdList.SetViewport(0, 0, 0.0f, SourceTexture2D->GetSizeX(), SourceTexture2D->GetSizeY(), 1.0f);
        RHICmdList.DrawPrimitive(0, 2, 1);
        RHICmdList.EndRenderPass();

        RHICmdList.Transition(FRHITransitionInfo(DestTexture2D, ERHIAccess::Unknown, ERHIAccess::CopyDest));
    }
}
