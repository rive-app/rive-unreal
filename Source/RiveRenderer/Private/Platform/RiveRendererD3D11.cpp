// Copyright Rive, Inc. All rights reserved.

#include "RiveRendererD3D11.h"

#include "Logs/RiveRendererLog.h"

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

namespace UE::Rive::Renderer::Private
{
	class FRiveRendererD3D11GPUAdaptor
	{
	public:
		FRiveRendererD3D11GPUAdaptor()
		{
		}

		void Initialize(rive::pls::PLSRenderContextD3DImpl::ContextOptions& OutContextOptions)
		{
			check(IsInRenderingThread());

			D3D11RHI = GetID3D11DynamicRHI();
			D3D11DevicePtr = D3D11RHI->RHIGetDevice();
			D3D11DeviceContext = nullptr;
			D3D11DevicePtr->GetImmediateContext(&D3D11DeviceContext);

			if (SUCCEEDED(D3D11DevicePtr->QueryInterface(__uuidof(IDXGIDevice), (void**)&DXGIDevice)))
			{
				IDXGIAdapter* DXGIAdapter;
				if (SUCCEEDED(DXGIDevice->GetAdapter(&DXGIAdapter)))
				{
					DXGI_ADAPTER_DESC AdapterDesc;
					DXGIAdapter->GetDesc(&AdapterDesc);

					OutContextOptions.isIntel = AdapterDesc.VendorId == 0x163C ||
						AdapterDesc.VendorId == 0x8086 || AdapterDesc.VendorId == 0x8087;

					DXGIAdapter->Release();
				}
			}

			// Init Bland State to fix the issue with Slate UI rendering
			InitBlandState();
		}

		ID3D11DynamicRHI* GetD3D11RHI() const { return D3D11RHI; }
		ID3D11Device* GetD3D11DevicePtr() const { return D3D11DevicePtr; }
		ID3D11DeviceContext* GetD3D11DeviceContext() const { return D3D11DeviceContext; }
		IDXGIDevice* GetDXGIDevice() const { return DXGIDevice; }

	private:
		void InitBlandState()
		{
			check(IsInRenderingThread());
			check(D3D11DevicePtr);
			
			D3D11_BLEND_DESC BlendDesc;
			BlendDesc.AlphaToCoverageEnable = false;
			BlendDesc.IndependentBlendEnable = false;
			BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

			// dest.a = 1-(1-dest.a)*src.a + dest.a
			BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
			BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

			BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			BlendDesc.RenderTarget[0].BlendEnable = true;

			HRESULT Hr = D3D11DevicePtr->CreateBlendState( &BlendDesc, AlphaBlendState.GetInitReference() );
			if (FAILED(Hr))
			{
				UE_LOG(LogRiveRenderer, Warning, TEXT("D3D11DevicePtr->CreateBlendState( &BlendDesc, AlphaBlendState.GetInitReference() )"));
				return;
			}

			D3D11_RASTERIZER_DESC RasterDesc;
			RasterDesc.CullMode = D3D11_CULL_NONE;
			RasterDesc.FillMode = D3D11_FILL_SOLID;
			RasterDesc.FrontCounterClockwise = true;
			RasterDesc.DepthBias = 0;
			RasterDesc.DepthBiasClamp = 0;
			RasterDesc.SlopeScaledDepthBias = 0;
			RasterDesc.ScissorEnable = false;
			RasterDesc.MultisampleEnable = false;
			RasterDesc.AntialiasedLineEnable = false;

			Hr = D3D11DevicePtr->CreateRasterizerState( &RasterDesc, NormalRasterState.GetInitReference() );
		}
	private:
		ID3D11DynamicRHI* D3D11RHI = nullptr;
		ID3D11Device* D3D11DevicePtr = nullptr;
		ID3D11DeviceContext* D3D11DeviceContext = nullptr;
		IDXGIDevice* DXGIDevice = nullptr;

	public:
		TRefCountPtr<ID3D11BlendState> AlphaBlendState;
		TRefCountPtr<ID3D11RasterizerState> NormalRasterState;
	};
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
		rive::pls::PLSRenderContextD3DImpl::ContextOptions ContextOptions;

		D3D11GPUAdaptor = MakeUnique<UE::Rive::Renderer::Private::FRiveRendererD3D11GPUAdaptor>();
		D3D11GPUAdaptor->Initialize(ContextOptions);

#if WITH_RIVE
		PLSRenderContext = rive::pls::PLSRenderContextD3DImpl::MakeContext(D3D11GPUAdaptor->GetD3D11DevicePtr(), D3D11GPUAdaptor->GetD3D11DeviceContext(), ContextOptions);
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

	PLSRenderer = std::make_unique<rive::pls::PLSRenderer>(PLSRenderContext.get());
}

void UE::Rive::Renderer::Private::FRiveRendererD3D11::ResetBlendState() const
{
	check(IsInRenderingThread());
	check(D3D11GPUAdaptor.IsValid());

	ID3D11DeviceContext* D3D11DeviceContext = D3D11GPUAdaptor->GetD3D11DeviceContext();
	if (!D3D11DeviceContext)
	{
		UE_LOG(LogRiveRenderer, Warning, TEXT("D3D11DeviceContext is nullptr while reset of bland state"));
	}

	// Reference is here FSlateD3D11RenderingPolicy::DrawElements(), set the blend state back to whatever Unreal expects after Rive renderers 
	D3D11DeviceContext->OMSetBlendState( D3D11GPUAdaptor->AlphaBlendState, 0, 0xFFFFFFFF );

	// Reset the Raster state when finished, there are places that are making assumptions about the raster state
	// that need to be fixed.
	D3D11DeviceContext->RSSetState(D3D11GPUAdaptor->NormalRasterState);
}

#endif // WITH_RIVE

UE_ENABLE_OPTIMIZATION

#endif // PLATFORM_WINDOWS