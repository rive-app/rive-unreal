// Copyright Rive, Inc. All rights reserved.

#include "RiveRendererD3D11.h"

#if PLATFORM_WINDOWS
#include "ID3D11DynamicRHI.h"
#include "Logs/RiveRendererLog.h"
#include "ProfilingDebugging/RealtimeGPUProfiler.h"
#include "RiveRenderTargetD3D11.h"
#include "Windows/D3D11ThirdParty.h"

#if WITH_RIVE
#include "RiveCore/Public/PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#include "rive/pls/d3d/pls_render_context_d3d_impl.hpp"
#include "rive/pls/pls_renderer.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

// ------------- FRiveRendererD3D11GPUAdapter -------------
#if WITH_RIVE
void UE::Rive::Renderer::Private::FRiveRendererD3D11GPUAdapter::Initialize(rive::pls::PLSRenderContextD3DImpl::ContextOptions& OutContextOptions)
{
	check(IsInRenderingThread());

	D3D11RHI = GetID3D11DynamicRHI();
	D3D11DevicePtr = D3D11RHI->RHIGetDevice();
	D3D11DeviceContext = nullptr;
	D3D11DevicePtr->GetImmediateContext(&D3D11DeviceContext);

	if (SUCCEEDED(D3D11DevicePtr->QueryInterface(__uuidof(IDXGIDevice), (void**)DXGIDevice.GetInitReference())))
	{
		TRefCountPtr<IDXGIAdapter> DXGIAdapter;
		if (SUCCEEDED(DXGIDevice->GetAdapter(DXGIAdapter.GetInitReference())))
		{
			DXGI_ADAPTER_DESC AdapterDesc;
			DXGIAdapter->GetDesc(&AdapterDesc);

			OutContextOptions.isIntel = AdapterDesc.VendorId == 0x163C ||
				AdapterDesc.VendorId == 0x8086 || AdapterDesc.VendorId == 0x8087;
		}
	}

	// Init Blend State to fix the issue with Slate UI rendering
	InitBlendState();
}
#endif // WITH_RIVE

void UE::Rive::Renderer::Private::FRiveRendererD3D11GPUAdapter::ResetDXStateForRive()
{
	if (!D3D11DeviceContext)
	{
		UE_LOG(LogRiveRenderer, Warning, TEXT("D3D11DeviceContext is nullptr while resetting the DirectX state"));
		return;
	}

	// Rive is resetting the Rasterizer and BlendState in PLSRenderContextD3DImpl::flush, so no need to call them
	// the DepthStencil is not set by Rive, so we reset to default
	D3D11DeviceContext->OMSetDepthStencilState(nullptr, 0);
}

void UE::Rive::Renderer::Private::FRiveRendererD3D11GPUAdapter::ResetDXState()
{
	if (!D3D11DeviceContext)
	{
		UE_LOG(LogRiveRenderer, Warning, TEXT("D3D11DeviceContext is nullptr while resetting the DirectX state"));
		return;
	}
	
	// We don't need to set a BlendState because we can match the default with TStaticBlendState<>::GetRHI(); 
	D3D11DeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
	// We need a Rasterizer because the default state sets D3D11_RASTERIZER_DESC::FrontCounterClockwise to false, but UE sets it to true
	D3D11DeviceContext->RSSetState(DefaultRasterState);
	// We also need a DepthStencil because the default raster state states D3D11_DEPTH_STENCIL_DESC::DepthWriteMask to D3D11_DEPTH_WRITE_MASK_ALL, but UE crashed with that option
	D3D11DeviceContext->OMSetDepthStencilState(DefaultDepthStencilState, 0);
}

void UE::Rive::Renderer::Private::FRiveRendererD3D11GPUAdapter::InitBlendState()
{
	check(IsInRenderingThread());
	check(D3D11DevicePtr);

	// Below is matching TStaticRasterizerState<>::GetRHI();
	D3D11_RASTERIZER_DESC RasterDesc;
	FMemory::Memzero(&RasterDesc,sizeof(D3D11_RASTERIZER_DESC));
	RasterDesc.CullMode = D3D11_CULL_NONE;
	RasterDesc.FillMode = D3D11_FILL_SOLID;
	RasterDesc.SlopeScaledDepthBias = 0.f;
	RasterDesc.FrontCounterClockwise = true;
	RasterDesc.DepthBias = 0.f;
	RasterDesc.DepthClipEnable = true;
	RasterDesc.MultisampleEnable = true;
	RasterDesc.ScissorEnable = true;

	RasterDesc.AntialiasedLineEnable = false;
	RasterDesc.DepthBiasClamp = 0.f;

	HRESULT Hr = D3D11DevicePtr->CreateRasterizerState(&RasterDesc, DefaultRasterState.GetInitReference());
	if (FAILED(Hr))
	{
		UE_LOG(LogRiveRenderer, Warning, TEXT("D3D11DevicePtr->CreateRasterizerState( &RasterDesc, NormalRasterState.GetInitReference() ) Failed"));
	}

	// Below is matching TStaticDepthStencilState<false>::GetRHI();
	D3D11_DEPTH_STENCIL_DESC DepthStencilDesc;
	FMemory::Memzero(&DepthStencilDesc,sizeof(D3D11_DEPTH_STENCIL_DESC));
	DepthStencilDesc.DepthEnable = true; // because DepthTest = CF_DepthNearOrEqual
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS; // CF_DepthNearOrEqual has no match
	
	DepthStencilDesc.StencilEnable = false;
	DepthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS ;
	DepthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.BackFace = DepthStencilDesc.FrontFace;
	DepthStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	DepthStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	
	Hr = D3D11DevicePtr->CreateDepthStencilState( &DepthStencilDesc, DefaultDepthStencilState.GetInitReference() );
	if (FAILED(Hr))
	{
		UE_LOG(LogRiveRenderer, Warning, TEXT("D3D11DevicePtr->CreateDepthStencilState( &DepthStencilDesc, DepthStencilState.GetInitReference() ) Failed"));
	}
}

// ------------- FRiveRendererD3D11 -------------

TSharedPtr<UE::Rive::Renderer::IRiveRenderTarget> UE::Rive::Renderer::Private::FRiveRendererD3D11::CreateTextureTarget_GameThread(const FName& InRiveName, UTexture2DDynamic* InRenderTarget)
{
	check(IsInGameThread());

	FScopeLock Lock(&ThreadDataCS);

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
#if WITH_RIVE
		rive::pls::PLSRenderContextD3DImpl::ContextOptions ContextOptions;

		D3D11GPUAdapter = MakeUnique<UE::Rive::Renderer::Private::FRiveRendererD3D11GPUAdapter>();
		D3D11GPUAdapter->Initialize(ContextOptions);
		
		PLSRenderContext = rive::pls::PLSRenderContextD3DImpl::MakeContext(D3D11GPUAdapter->GetD3D11DevicePtr(), D3D11GPUAdapter->GetD3D11DeviceContext(), ContextOptions);
#endif // WITH_RIVE
	}
}

DECLARE_GPU_STAT_NAMED(CreatePLSRenderer, TEXT("CreatePLSRenderer_RenderThread"));
void UE::Rive::Renderer::Private::FRiveRendererD3D11::CreatePLSRenderer_RenderThread(FRHICommandListImmediate& RHICmdList)
{
	RIVE_DEBUG_FUNCTION_INDENT;
}

void UE::Rive::Renderer::Private::FRiveRendererD3D11::ResetDXStateForRive() const
{
	check(IsInRenderingThread());
	check(D3D11GPUAdapter.IsValid());

	FScopeLock Lock(&ThreadDataCS);
	D3D11GPUAdapter->ResetDXStateForRive();
}

void UE::Rive::Renderer::Private::FRiveRendererD3D11::ResetDXState() const
{
	check(IsInRenderingThread());
	check(D3D11GPUAdapter.IsValid());

	FScopeLock Lock(&ThreadDataCS);
	D3D11GPUAdapter->ResetDXState();
}

#endif // PLATFORM_WINDOWS
