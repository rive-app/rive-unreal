// Copyright Rive, Inc. All rights reserved.

#include "RiveRendererD3D11.h"

#if PLATFORM_WINDOWS
#include "ID3D11DynamicRHI.h"
#include "Logs/RiveRendererLog.h"
#include "ProfilingDebugging/RealtimeGPUProfiler.h"
#include "RiveRenderTargetD3D11.h"
#include "Windows/D3D11ThirdParty.h"
#include "D3D11RHIPrivate.h"

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
}
#endif // WITH_RIVE

void UE::Rive::Renderer::Private::FRiveRendererD3D11GPUAdapter::ResetDXState()
{
	// Clear the internal state kept by UE and reset DX
	FD3D11DynamicRHI* DynRHI = static_cast<FD3D11DynamicRHI*>(D3D11RHI);
	if (ensure(DynRHI))
	{
		DynRHI->ClearState();
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

void UE::Rive::Renderer::Private::FRiveRendererD3D11::ResetDXState() const
{
	check(IsInRenderingThread());
	check(D3D11GPUAdapter.IsValid());

	FScopeLock Lock(&ThreadDataCS);
	D3D11GPUAdapter->ResetDXState();
}

#endif // PLATFORM_WINDOWS
