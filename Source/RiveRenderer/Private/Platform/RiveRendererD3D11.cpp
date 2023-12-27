// Copyright Rive, Inc. All rights reserved.

#include "RiveRendererD3D11.h"

#include "CanvasTypes.h"
#include "RiveRenderTargetD3D11.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Windows/D3D11ThirdParty.h"
#include "ID3D11DynamicRHI.h"
#include "rive/artboard.hpp"

#include "rive/pls/d3d/pls_render_context_d3d_impl.hpp"
#include "rive/pls/pls_renderer.hpp"

UE_DISABLE_OPTIMIZATION

UE::Rive::Renderer::Private::FRiveRendererD3D11::FRiveRendererD3D11()
{
}

UE::Rive::Renderer::Private::FRiveRendererD3D11::~FRiveRendererD3D11()
{
}

DECLARE_GPU_STAT_NAMED(DebugColorDraw, TEXT("FRiveRendererD3D11::DebugColorDraw"));
void UE::Rive::Renderer::Private::FRiveRendererD3D11::DebugColorDraw(UTextureRenderTarget2D* InTexture, const FLinearColor DebugColor, rive::Artboard* InNativeArtBoard)
{
	check(IsInGameThread());

	FScopeLock Lock(&ThreadDataCS);

	FTextureRenderTargetResource* RenderTargetResource = InTexture->GameThread_GetRenderTargetResource();

	ENQUEUE_RENDER_COMMAND(DebugColorDraw)(
[this, InNativeArtBoard, RenderTargetResource, DebugColor](FRHICommandListImmediate& RHICmdList)
	{
		SCOPED_GPU_STAT(RHICmdList, DebugColorDraw);
		check(IsInRenderingThread());

		FScopeLock Lock(&ThreadDataCS);

		if (PLSRenderContext == nullptr)
		{
			return;
		}

		rive::pls::PLSRenderContext::FrameDescriptor frameDescriptor;
		frameDescriptor.renderTarget = TestPLSRenderTargetD3D;
		frameDescriptor.loadAction = rive::pls::LoadAction::clear;
		frameDescriptor.clearColor = 0x00000000;
		frameDescriptor.wireframe = false;
		frameDescriptor.fillsDisabled = false;
		frameDescriptor.strokesDisabled = false;

	    std::unique_ptr<rive::pls::PLSRenderer> PLSRenderer = std::make_unique<rive::pls::PLSRenderer>(PLSRenderContext.get());

		InNativeArtBoard->draw(PLSRenderer.get());

		static bool bOnce = false;
		if (bOnce == false)
		{
			// JUST for testing here
			FCanvas Canvas(RenderTargetResource, nullptr, FGameTime(), GMaxRHIFeatureLevel);

			Canvas.Clear(DebugColor);

			bOnce = true;
		}

	});
}

void UE::Rive::Renderer::Private::FRiveRendererD3D11::SetTextureTarget_GameThread(const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget)
{
	check(IsInGameThread());

	FScopeLock Lock(&ThreadDataCS);

	FTextureRenderTargetResource* RenderTargetResource = InRenderTarget->GameThread_GetRenderTargetResource();
	ENQUEUE_RENDER_COMMAND(CacheTextureTarget_RenderThread)(
	[RenderTargetResource, this](FRHICommandListImmediate& RHICmdList)
	{
		CacheTextureTarget_RenderThread(RHICmdList, RenderTargetResource->TextureRHI->GetTexture2D());
	});
	
	RenderTargets.Add(InRiveName, MakeShared<FRiveRenderTargetD3D11>());
}

DECLARE_GPU_STAT_NAMED(CacheTextureTarget, TEXT("FRiveRendererD3D11::CacheTextureTarget_RenderThread"));
void UE::Rive::Renderer::Private::FRiveRendererD3D11::CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource)
{
	check(IsInRenderingThread());

	FScopeLock Lock(&ThreadDataCS);

	if (PLSRenderContext == nullptr)
	{
		return;
	}

	// TODO, make sure we have correct texture DXGI_FORMAT_R8G8B8A8_UNORM
	EPixelFormat PixelFormat = InRHIResource->GetFormat();
	if (PixelFormat != PF_R8G8B8A8)
	{
		return;
	}

	SCOPED_GPU_STAT(RHICmdList, CacheTextureTarget);
	if (IsRHID3D11() && InRHIResource.IsValid())
	{
		ID3D11DynamicRHI* D3D11RHI = GetID3D11DynamicRHI();
		ID3D11Device* D3D11DevicePtr = D3D11RHI->RHIGetDevice();
		ID3D11Texture2D* D3D11ResourcePtr = (ID3D11Texture2D*)D3D11RHI->RHIGetResource(InRHIResource);
		D3D11_TEXTURE2D_DESC desc;
		D3D11ResourcePtr->GetDesc(&desc);
		UE_LOG(LogTemp, Warning, TEXT("D3D11ResourcePtr texture %dx%d"), desc.Width, desc.Height);



		// For now we just set one renderer and one texture
		rive::pls::PLSRenderContextD3DImpl* const PLSRenderContextD3DImpl = PLSRenderContext->static_impl_cast<rive::pls::PLSRenderContextD3DImpl>();
		TestPLSRenderTargetD3D = PLSRenderContextD3DImpl->makeRenderTarget(desc.Width, desc.Height);
		TestPLSRenderTargetD3D->setTargetTexture(D3D11DevicePtr, D3D11ResourcePtr);
	}
}

DECLARE_GPU_STAT_NAMED(CreatePLSContext, TEXT("CreatePLSContext_RenderThread"));
void UE::Rive::Renderer::Private::FRiveRendererD3D11::CreatePLSContext_RenderThread(FRHICommandListImmediate& RHICmdList)
{
	check(IsInRenderingThread());

	FScopeLock Lock(&ThreadDataCS);

	SCOPED_GPU_STAT(RHICmdList, CreatePLSContext);
	if (IsRHID3D11())
	{
		bool bIsIntel = false;

		ID3D11DynamicRHI* D3D11RHI = GetID3D11DynamicRHI();
		ID3D11Device* D3D11DevicePtr = D3D11RHI->RHIGetDevice();

		ID3D11DeviceContext* D3D11DeviceContext = nullptr;
		D3D11DevicePtr->GetImmediateContext(&D3D11DeviceContext);

		IDXGIDevice* DXGIDevice;
		if (SUCCEEDED(D3D11DevicePtr->QueryInterface(__uuidof(IDXGIDevice), (void**)&DXGIDevice)))
		{
			IDXGIAdapter* DXGIAdapter;
			DXGI_ADAPTER_DESC AdapterDesc{};
			if (SUCCEEDED(DXGIDevice->GetAdapter(&DXGIAdapter)))
			{
				DXGIAdapter->GetDesc(&AdapterDesc);
				bIsIntel = AdapterDesc.VendorId == 0x163C ||
						  AdapterDesc.VendorId == 0x8086 || AdapterDesc.VendorId == 0x8087;
				DXGIAdapter->Release();
			}
		}

		PLSRenderContext = rive::pls::PLSRenderContextD3DImpl::MakeContext(D3D11DevicePtr, D3D11DeviceContext, bIsIntel);
	}
}

DECLARE_GPU_STAT_NAMED(CreatePLSRenderer, TEXT("CreatePLSRenderer_RenderThread"));
void UE::Rive::Renderer::Private::FRiveRendererD3D11::CreatePLSRenderer_RenderThread(FRHICommandListImmediate& RHICmdList)
{
	if (PLSRenderContext == nullptr)
	{
		return;
	}

	check(IsInRenderingThread());

	FScopeLock Lock(&ThreadDataCS);

	SCOPED_GPU_STAT(RHICmdList, CreatePLSRenderer);

	//PLSRenderer = std::make_unique<rive::pls::PLSRenderer>(PLSRenderContext.get());
}

UE_ENABLE_OPTIMIZATION
