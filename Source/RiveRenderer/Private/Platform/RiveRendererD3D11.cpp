// Copyright Rive, Inc. All rights reserved.

#include "RiveRendererD3D11.h"

#if PLATFORM_WINDOWS

#include "Engine/TextureRenderTarget2D.h"
#include "RiveRenderTargetD3D11.h"
#include "Windows/D3D11ThirdParty.h"
#include "ID3D11DynamicRHI.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/pls/d3d/pls_render_context_d3d_impl.hpp"
#include "rive/pls/pls_renderer.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

UE_DISABLE_OPTIMIZATION

UE::Rive::Renderer::Private::FRiveRendererD3D11::FRiveRendererD3D11()
{
}

TSharedPtr<UE::Rive::Renderer::IRiveRenderTarget> UE::Rive::Renderer::Private::FRiveRendererD3D11::CreateTextureTarget_GameThread(const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget)
{
	check(IsInGameThread());
	
	const TSharedPtr<FRiveRenderTargetD3D11> RiveRenderTarget = MakeShared<FRiveRenderTargetD3D11>(SharedThis(this), InRiveName, InRenderTarget);
	
	RenderTargets.Add(InRiveName, RiveRenderTarget);

	return RiveRenderTarget;
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

			DXGI_ADAPTER_DESC AdapterDesc;

			if (SUCCEEDED(DXGIDevice->GetAdapter(&DXGIAdapter)))
			{
				DXGIAdapter->GetDesc(&AdapterDesc);

				bIsIntel = AdapterDesc.VendorId == 0x163C ||
						  AdapterDesc.VendorId == 0x8086 || AdapterDesc.VendorId == 0x8087;

				DXGIAdapter->Release();
			}
		}

#if WITH_RIVE

		PLSRenderContext = rive::pls::PLSRenderContextD3DImpl::MakeContext(D3D11DevicePtr, D3D11DeviceContext, bIsIntel);

#endif // WITH_RIVE
	}
}

DECLARE_GPU_STAT_NAMED(CreatePLSRenderer, TEXT("CreatePLSRenderer_RenderThread"));
void UE::Rive::Renderer::Private::FRiveRendererD3D11::CreatePLSRenderer_RenderThread(FRHICommandListImmediate& RHICmdList)
{
#if WITH_RIVE

	if (PLSRenderContext == nullptr)
	{
		return;
	}

	check(IsInRenderingThread());

	FScopeLock Lock(&ThreadDataCS);

	SCOPED_GPU_STAT(RHICmdList, CreatePLSRenderer);

	rive::pls::PLSRenderContext::FrameDescriptor FrameDescriptor;

	FrameDescriptor.renderTarget = nullptr;
	
	FrameDescriptor.loadAction = rive::pls::LoadAction::clear;
	
	FrameDescriptor.clearColor = 0x00000000;
	
	FrameDescriptor.wireframe = false;
	
	FrameDescriptor.fillsDisabled = false;
	
	FrameDescriptor.strokesDisabled = false;

	PLSRenderContext->beginFrame(std::move(FrameDescriptor));

	PLSRenderer = std::make_unique<rive::pls::PLSRenderer>(PLSRenderContext.get());

#endif // WITH_RIVE
}

UE_ENABLE_OPTIMIZATION

#endif // PLATFORM_WINDOWS
