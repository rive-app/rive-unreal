// Copyright Rive, Inc. All rights reserved.

#include "RiveRenderTargetOpenGL.h"

#if PLATFORM_ANDROID
#include "OpenGLDrv.h"
#include "IRiveRendererModule.h"
#include "RiveRendererOpenGL.h"
#include "RiveRenderer.h"
#include "Engine/Texture2DDynamic.h"
#include "Logs/RiveRendererLog.h"
#include "RiveRenderCommand.h"
#include "IOpenGLDynamicRHI.h"
#include "TextureResource.h"


#if WITH_RIVE
#include "RiveCore/Public/PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#include "rive/artboard.hpp"
#include "rive/pls/pls_renderer.hpp"
#include "rive/pls/gl/pls_render_context_gl_impl.hpp"
#include "rive/pls/gl/pls_render_target_gl.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::FRiveRenderTargetOpenGL(const TSharedRef<FRiveRendererOpenGL>& InRiveRenderer, const FName& InRiveName, UTexture2DDynamic* InRenderTarget)
	: FRiveRenderTarget(InRiveRenderer, InRiveName, InRenderTarget), RiveRendererGL(InRiveRenderer)
{
	RIVE_DEBUG_FUNCTION_INDENT;
}

UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::~FRiveRenderTargetOpenGL()
{
	RIVE_DEBUG_FUNCTION_INDENT;
	CachedPLSRenderTargetOpenGL.reset();
}

void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::Initialize()
{
	RIVE_DEBUG_FUNCTION_INDENT;
	check(IsInGameThread());
	
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

	FTextureResource* RenderTargetResource = RenderTarget->GetResource();
	if (IRiveRendererModule::RunInGameThread())
	{
		CacheTextureTarget_Internal(RenderTarget->GetResource()->TextureRHI);
	}
	else
	{
		ENQUEUE_RENDER_COMMAND(CacheTextureTarget_RenderThread)(
		[RenderTargetResource, this](FRHICommandListImmediate& RHICmdList)
		{
			CacheTextureTarget_RenderThread(RHICmdList, RenderTargetResource->TextureRHI);
		});
	}
}

DECLARE_GPU_STAT_NAMED(CacheTextureTarget, TEXT("FRiveRenderTargetOpenGL::CacheTextureTarget_RenderThread"));
void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InTexture)
{
	RIVE_DEBUG_FUNCTION_INDENT;
	check(IsInRenderingThread());

	if (IRiveRendererModule::RunInGameThread())
	{
		AsyncTask(ENamedThreads::GameThread,[this, InTexture]()
		{
			CacheTextureTarget_Internal(InTexture);
		});
	}
	else
	{
		SCOPED_GPU_STAT(RHICmdList, CacheTextureTarget);
		RHICmdList.EnqueueLambda([this, InTexture](FRHICommandListImmediate& RHICmdList)
		{
			CacheTextureTarget_Internal(InTexture);
		});
	}
}

void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::Submit()
{
	RIVE_DEBUG_FUNCTION_INDENT;
	check(IsInGameThread());
	
	if (IRiveRendererModule::RunInGameThread())
	{
		Render_Internal(RenderCommands);
	}
	else
	{
		FRiveRenderTarget::Submit();
	}
}

rive::rcp<rive::pls::PLSRenderTarget> UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::GetRenderTarget() const
{
	return CachedPLSRenderTargetOpenGL;
}

std::unique_ptr<rive::pls::PLSRenderer> UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::BeginFrame()
{
	RIVE_DEBUG_FUNCTION_INDENT;
	check(IsInGameThread() || IsInRHIThread());
	ENABLE_VERIFY_GL_THREAD;

	rive::pls::PLSRenderContext* PLSRenderContextPtr = RiveRenderer->GetPLSRenderContextPtr();
	if (PLSRenderContextPtr == nullptr)
	{
		return nullptr;
	}
	
	RIVE_DEBUG_VERBOSE("PLSRenderContextGLImpl->resetGLState() %p", PLSRenderContextPtr);
	PLSRenderContextPtr->static_impl_cast<rive::pls::PLSRenderContextGLImpl>()->invalidateGLState();
	
	return FRiveRenderTarget::BeginFrame();
}

void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::EndFrame() const
{
	RIVE_DEBUG_FUNCTION_INDENT;
	check(IsInGameThread() || IsInRHIThread());
	ENABLE_VERIFY_GL_THREAD;
	
	rive::pls::PLSRenderContext* PLSRenderContextPtr = RiveRenderer->GetPLSRenderContextPtr();
	if (PLSRenderContextPtr == nullptr)
	{
		return;
	}

	// End drawing a frame.
	// Flush
	RIVE_DEBUG_VERBOSE("PLSRenderContextPtr->flush()  %p", PLSRenderContextPtr);
	const rive::pls::PLSRenderContext::FlushResources FlushResources
	{
		GetRenderTarget().get()
	};
	PLSRenderContextPtr->flush(FlushResources);

	//todo: android texture blink if we don't call glReadPixels? to investigate
	TArray<FIntVector2> Points{ {0,0}, { 100,100 }, { 200,200 }, { 300,300 }, { 400,400 }, { 500,500 }, { 600,600 }, { 700,700 } };
	for (FIntVector2 Point : Points) 
	{
		if (Point.X < GetWidth() && Point.Y < GetHeight())
		{
			GLubyte pix[4];
			glReadPixels(Point.X, Point.Y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pix);
			RIVE_DEBUG_VERBOSE("Pixel [%d,%d] = %u %u %u %u", Point.X, Point.Y, pix[0], pix[1], pix[2], pix[3])
		}
	}
	
	// Reset
	RIVE_DEBUG_VERBOSE("PLSRenderContextPtr->unbindGLInternalResources() %p", PLSRenderContextPtr);
	PLSRenderContextPtr->static_impl_cast<rive::pls::PLSRenderContextGLImpl>()->unbindGLInternalResources(); //careful, need UE's internal state to match
	// PLSRenderContextPtr->static_impl_cast<rive::pls::PLSRenderContextGLImpl>()->invalidateGLState(); //careful, need UE's internal state to match

	ResetOpenGLState();
}

void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::Render_RenderThread(FRHICommandListImmediate& RHICmdList, const TArray<FRiveRenderCommand>& RiveRenderCommands)
{
	RIVE_DEBUG_FUNCTION_INDENT;
	check(IsInRenderingThread());
	
	RHICmdList.EnqueueLambda([this, RiveRenderCommands = RiveRenderCommands](FRHICommandListImmediate& RHICmdList)
	{
		// ResetOpenGLState();
		Render_Internal(RiveRenderCommands);
	});
}

#if WITH_RIVE

void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::CacheTextureTarget_Internal(const FTexture2DRHIRef& InRHIResource)
{
	RIVE_DEBUG_FUNCTION_INDENT;
	check(IsInGameThread() || IsInRHIThread());
	ENABLE_VERIFY_GL_THREAD;

	if (!ensure(FRiveRendererOpenGL::IsRHIOpenGL()) || !InRHIResource.IsValid())
	{
		UE_LOG(LogRiveRenderer, Error, TEXT("Error while caching the TextureTarget: RHI, OpenGL or Texture is not valid."));
		return;
	}
	
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
	
#if WITH_RIVE
	rive::pls::PLSRenderContext* PLSRenderContext = RiveRendererGL->GetOrCreatePLSRenderContextPtr_Internal();

	if (PLSRenderContext == nullptr)
	{
		UE_LOG(LogRiveRenderer, Error, TEXT("PLSRenderContext is null"));
		return;
	}
	RIVE_DEBUG_VERBOSE("PLSRenderContext is valid");
#endif // WITH_RIVE

	const IOpenGLDynamicRHI* OpenGlRHI = GetIOpenGLDynamicRHI();
	GLuint OpenGLResourcePtr = (GLuint)OpenGlRHI->RHIGetResource(InRHIResource);
	RIVE_DEBUG_VERBOSE("OpenGLResourcePtr  %d", OpenGLResourcePtr);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, OpenGLResourcePtr);
	GLint format;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	RIVE_DEBUG_VERBOSE("GL_TEXTURE_INTERNAL_FORMAT: %x (d: %d)  is GL_RGBA8? %s", format, format, format == GL_RGBA8 ? TEXT("TRUE") : TEXT("FALSE"));
	
	if (format != GL_RGBA8)
	{
		const FString PixelFormatString = GetPixelFormatString(InRHIResource->GetFormat());
		UE_LOG(LogRiveRenderer, Error, TEXT("Texture Target has the wrong Pixel Format (not GL_RGBA8 %x (d: %d)), PixelFormat is '%s'. GL_TEXTURE_INTERNAL_FORMAT: %x (d: %d)"), GL_RGBA8, GL_RGBA8, *PixelFormatString, format, format);
		return;
	}

#if WITH_RIVE
	// For now we just set one renderer and one texture
	rive::pls::PLSRenderContextGLImpl* PLSRenderContextGLImpl = PLSRenderContext->static_impl_cast<rive::pls::PLSRenderContextGLImpl>();
	RIVE_DEBUG_VERBOSE("PLSRenderContextGLImpl->resetGLState()");
	PLSRenderContextGLImpl->invalidateGLState();

	int w, h;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, OpenGLResourcePtr);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

	if (w == 0 || h == 0)
	{
		UE_LOG(LogRiveRenderer, Error, TEXT("The size of the Texture Target is 0!   w: %d, h: %d"), w, h);
		return;
	}
	RIVE_DEBUG_VERBOSE("OpenGLResourcePtr %d texture size %dx%d", OpenGLResourcePtr, w, h);

	if (CachedPLSRenderTargetOpenGL)
	{
		CachedPLSRenderTargetOpenGL.reset();
	}
	
	CachedPLSRenderTargetOpenGL = rive::make_rcp<rive::pls::TextureRenderTargetGL>(w, h);
	RIVE_DEBUG_VERBOSE("PLSRenderContextGLImpl->setTargetTexture( %d )", OpenGLResourcePtr);
	CachedPLSRenderTargetOpenGL->setTargetTexture(OpenGLResourcePtr);
	RIVE_DEBUG_VERBOSE("CachedPLSRenderTargetOpenGL set to  %p", CachedPLSRenderTargetOpenGL.get());

	ResetOpenGLState();
#endif // WITH_RIVE
}

void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::ResetOpenGLState() const
{
	if (IsInRHIThread()) //todo: still not working, to be looked at
	{
		FOpenGLDynamicRHI* OpenGLDynamicRHI = static_cast<FOpenGLDynamicRHI*>(GetIOpenGLDynamicRHI());
		// OpenGLDynamicRHI->RHIInvalidateCachedState(); // lose the 3D ground

		FOpenGLContextState& ContextState = OpenGLDynamicRHI->GetContextStateForCurrentContext(true);
		
		RIVE_DEBUG_VERBOSE("Pre RHIPostExternalCommandsReset");
		//// Manual reset of GL commands to match the GL State before Rive Commands. Supposed to be handled by RHIPostExternalCommandsReset, TBC
		//// FOpenGL::BindProgramPipeline(ContextState.Program);
		//// glViewport(ContextState.Viewport.Min.X, ContextState.Viewport.Min.Y, ContextState.Viewport.Max.X - ContextState.Viewport.Min.X, ContextState.Viewport.Max.Y - ContextState.Viewport.Min.Y);
		//// FOpenGL::DepthRange(ContextState.DepthMinZ, ContextState.DepthMaxZ);
		//// ContextState.bScissorEnabled ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);
		//// glScissor(ContextState.Scissor.Min.X, ContextState.Scissor.Min.Y, ContextState.Scissor.Max.X - ContextState.Scissor.Min.X, ContextState.Scissor.Max.Y - ContextState.Scissor.Min.Y);
		//glBindFramebuffer(GL_FRAMEBUFFER, ContextState.Framebuffer);
		//// glBindBuffer(GL_ARRAY_BUFFER, ContextState.ArrayBufferBound);
		//// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ContextState.ElementArrayBufferBound);

		// from UpdateRasterizerStateInOpenGLContext
		// {
		// 	if (FOpenGL::SupportsPolygonMode())
		// 	{
		// 		FOpenGL::PolygonMode(GL_FRONT_AND_BACK, ContextState.RasterizerState.FillMode);
		// 	}
		// 	if (ContextState.RasterizerState.CullMode != GL_NONE)
		// 	{
		// 		glEnable(GL_CULL_FACE);
		// 		glCullFace(ContextState.RasterizerState.CullMode);
		// 	}
		// 	else
		// 	{
		// 		glDisable(GL_CULL_FACE);
		// 	}
		// 	if (FOpenGL::SupportsDepthClamp())
		// 	{
		// 		if (ContextState.RasterizerState.DepthClipMode == ERasterizerDepthClipMode::DepthClamp)
		// 		{
		// 			glEnable(GL_DEPTH_CLAMP);
		// 		}
		// 		else
		// 		{
		// 			glDisable(GL_DEPTH_CLAMP);
		// 		}
		// 	}
		// 	// Convert our platform independent depth bias into an OpenGL depth bias.
		// 	const float BiasScale = float((1<<24)-1);	// Warning: this assumes depth bits == 24, and won't be correct with 32.
		// 	float DepthBias = ContextState.RasterizerState.DepthBias * BiasScale;
		// 	
		// 	if ((DepthBias == 0.0f) && (ContextState.RasterizerState.SlopeScaleDepthBias == 0.0f))
		// 	{
		// 		// If we're here, both previous 2 'if' conditions are true, and this implies that cached state was not all zeroes, so we need to glDisable.
		// 		glDisable(GL_POLYGON_OFFSET_FILL);
		// 		if ( FOpenGL::SupportsPolygonMode() )
		// 		{
		// 			glDisable(GL_POLYGON_OFFSET_LINE);
		// 			glDisable(GL_POLYGON_OFFSET_POINT);
		// 		}
		// 	}
		// 	else
		// 	{
		// 		if (ContextState.RasterizerState.DepthBias == 0.0f && ContextState.RasterizerState.SlopeScaleDepthBias == 0.0f)
		// 		{
		// 			glEnable(GL_POLYGON_OFFSET_FILL);
		// 			if ( FOpenGL::SupportsPolygonMode() )
		// 			{
		// 				glEnable(GL_POLYGON_OFFSET_LINE);
		// 				glEnable(GL_POLYGON_OFFSET_POINT);
		// 			}
		// 		}
		// 		glPolygonOffset(ContextState.RasterizerState.SlopeScaleDepthBias, DepthBias);
		// 	}
		// } // ~from UpdateRasterizerStateInOpenGLContext
		//
		// { // from UpdateDepthStencilStateInOpenGLContext
		// 	if (ContextState.DepthStencilState.bZEnable)
		// 	{
		// 		glEnable(GL_DEPTH_TEST);
		// 	}
		// 	else
		// 	{
		// 		glDisable(GL_DEPTH_TEST);
		// 	}
		// 	glDepthMask(ContextState.DepthStencilState.bZWriteEnable);
		// 	glDepthFunc(ContextState.DepthStencilState.ZFunc);
		// 	if (ContextState.DepthStencilState.bStencilEnable)
		// 	{
		// 		glEnable(GL_STENCIL_TEST);
		// 	}
		// 	else
		// 	{
		// 		glDisable(GL_STENCIL_TEST);
		// 	}
		// 	{
		// 		// Invalidate cache to enforce update of part of stencil state that needs to be set with different functions, when needed next
		// 		// Values below are all invalid, but they'll never be used, only compared to new values to be set.
		// 		ContextState.DepthStencilState.StencilFunc = 0xFFFF;
		// 		ContextState.DepthStencilState.StencilFail = 0xFFFF;
		// 		ContextState.DepthStencilState.StencilZFail = 0xFFFF;
		// 		ContextState.DepthStencilState.StencilPass = 0xFFFF;
		// 		ContextState.DepthStencilState.CCWStencilFunc = 0xFFFF;
		// 		ContextState.DepthStencilState.CCWStencilFail = 0xFFFF;
		// 		ContextState.DepthStencilState.CCWStencilZFail = 0xFFFF;
		// 		ContextState.DepthStencilState.CCWStencilPass = 0xFFFF;
		// 		ContextState.DepthStencilState.StencilReadMask = 0xFFFF;
		// 	}
		// 	if (ContextState.DepthStencilState.bStencilEnable)
		// 	{
		// 		if (ContextState.DepthStencilState.bTwoSidedStencilMode)
		// 		{
		// 			glStencilFuncSeparate(GL_BACK, ContextState.DepthStencilState.StencilFunc, ContextState.StencilRef, ContextState.DepthStencilState.StencilReadMask);
		// 			glStencilOpSeparate(GL_BACK, ContextState.DepthStencilState.StencilFail, ContextState.DepthStencilState.StencilZFail, ContextState.DepthStencilState.StencilPass);
		// 			glStencilFuncSeparate(GL_FRONT, ContextState.DepthStencilState.CCWStencilFunc, ContextState.StencilRef, ContextState.DepthStencilState.StencilReadMask);
		// 			glStencilOpSeparate(GL_FRONT, ContextState.DepthStencilState.CCWStencilFail, ContextState.DepthStencilState.CCWStencilZFail, ContextState.DepthStencilState.CCWStencilPass);
		// 		}
		// 		else
		// 		{
		// 			glStencilFunc(ContextState.DepthStencilState.StencilFunc, ContextState.StencilRef, ContextState.DepthStencilState.StencilReadMask);
		// 			glStencilOp(ContextState.DepthStencilState.StencilFail, ContextState.DepthStencilState.StencilZFail, ContextState.DepthStencilState.StencilPass);
		// 		}
		// 		glStencilMask(ContextState.DepthStencilState.StencilWriteMask);
		// 	}
		// } // ~from UpdateDepthStencilStateInOpenGLContext
		
		// glBindVertexArray(0); // m_state->bindVAO(0);
		// //// glBindVertexBuffer(0);
		// glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ContextState.ElementArrayBufferBound); // m_state->bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		// glBindBuffer( GL_ARRAY_BUFFER, ContextState.ArrayBufferBound); //m_state->bindBuffer(GL_ARRAY_BUFFER, 0);
		// glBindBuffer( GL_UNIFORM_BUFFER, ContextState.UniformBufferBound); //m_state->bindBuffer(GL_UNIFORM_BUFFER, 0);
		// glBindBuffer( GL_PIXEL_UNPACK_BUFFER, ContextState.PixelUnpackBufferBound); //m_state->bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		// glBindFramebuffer(GL_FRAMEBUFFER, ContextState.Framebuffer);
		//// for (int i = 0; i <= CONTOUR_BUFFER_IDX; ++i)
		//// {
		//// 	glActiveTexture(GL_TEXTURE0 + kPLSTexIdxOffset + i);
		//// 	glBindTexture(GL_TEXTURE_2D, 0);
		//// }
		//
		// for (int i = 0; i < ContextState.Textures.Num(); i++)
		// {
		// 	glActiveTexture(GL_TEXTURE0 + i);
		// 	glBindTexture(GL_TEXTURE_2D, ContextState.Textures[i].Resource);
		// }
		// glActiveTexture(ContextState.ActiveTexture);
		
		// { // from SetPendingBlendStateForActiveRenderTargets
		// 	bool bABlendWasSet = false;
		// 	bool bMSAAEnabled = false;
		//
		// 	//
		// 	// Need to expand setting for glBlendFunction and glBlendEquation
		//
		// 	for (uint32 RenderTargetIndex = 0; RenderTargetIndex < MaxSimultaneousRenderTargets; ++RenderTargetIndex)
		// 	{
		// 		FOpenGLBlendStateData::FRenderTarget& RenderTargetBlendState = ContextState.BlendState.RenderTargets[RenderTargetIndex];
		// 		
		// 		if (RenderTargetBlendState.bAlphaBlendEnable)
		// 		{
		// 			FOpenGL::EnableIndexed(GL_BLEND, RenderTargetIndex);
		// 		}
		// 		else
		// 		{
		// 			FOpenGL::DisableIndexed(GL_BLEND, RenderTargetIndex);
		// 		}
		// 		
		// 		if (RenderTargetBlendState.bAlphaBlendEnable)
		// 		{
		// 			if (FOpenGL::SupportsSeparateAlphaBlend())
		// 			{
		// 				// Set current blend per stage
		// 				if (RenderTargetBlendState.bSeparateAlphaBlendEnable)
		// 				{
		// 					FOpenGL::BlendFuncSeparatei(
		// 						RenderTargetIndex,
		// 						RenderTargetBlendState.ColorSourceBlendFactor, RenderTargetBlendState.ColorDestBlendFactor,
		// 						RenderTargetBlendState.AlphaSourceBlendFactor, RenderTargetBlendState.AlphaDestBlendFactor
		// 					);
		// 					
		// 					FOpenGL::BlendEquationSeparatei(
		// 						RenderTargetIndex,
		// 						RenderTargetBlendState.ColorBlendOperation,
		// 						RenderTargetBlendState.AlphaBlendOperation
		// 					);
		// 				}
		// 				else
		// 				{
		// 					FOpenGL::BlendFunci(RenderTargetIndex, RenderTargetBlendState.ColorSourceBlendFactor, RenderTargetBlendState.ColorDestBlendFactor);
		// 					FOpenGL::BlendEquationi(RenderTargetIndex, RenderTargetBlendState.ColorBlendOperation);
		// 				}
		// 			}
		// 			else
		// 			{
		// 				if (bABlendWasSet)
		// 				{
		// 					// Detect the case of subsequent render target needing different blend setup than one already set in this call.
		// 				}
		// 				else
		// 				{
		// 					// Set current blend to all stages
		// 					if (RenderTargetBlendState.bSeparateAlphaBlendEnable)
		// 					{
		// 						glBlendFuncSeparate(
		// 							RenderTargetBlendState.ColorSourceBlendFactor, RenderTargetBlendState.ColorDestBlendFactor,
		// 							RenderTargetBlendState.AlphaSourceBlendFactor, RenderTargetBlendState.AlphaDestBlendFactor
		// 						);
		// 						glBlendEquationSeparate(
		// 							RenderTargetBlendState.ColorBlendOperation,
		// 							RenderTargetBlendState.AlphaBlendOperation
		// 						);
		// 					}
		// 					else
		// 					{
		// 						glBlendFunc(RenderTargetBlendState.ColorSourceBlendFactor, RenderTargetBlendState.ColorDestBlendFactor);
		// 						glBlendEquation(RenderTargetBlendState.ColorBlendOperation);
		// 					}
		// 					bABlendWasSet = true;
		// 				}
		// 			}
		// 		}
		// 		FOpenGL::ColorMaskIndexed(
		// 			RenderTargetIndex,
		// 			RenderTargetBlendState.ColorWriteMaskR,
		// 			RenderTargetBlendState.ColorWriteMaskG,
		// 			RenderTargetBlendState.ColorWriteMaskB,
		// 			RenderTargetBlendState.ColorWriteMaskA
		// 		);
		// 	}
		// 	
		// 	if (ContextState.bAlphaToCoverageEnabled)
		// 	{
		// 		glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		// 	}
		// 	else
		// 	{
		// 		glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		// 	}
		// } // ~from SetPendingBlendStateForActiveRenderTargets
		
		//glBindBuffer( GL_PIXEL_UNPACK_BUFFER, ContextState.PixelUnpackBufferBound);
		//// glBindBuffer( GL_UNIFORM_BUFFER, ContextState.UniformBufferBound);
		//glBindBuffer( GL_SHADER_STORAGE_BUFFER, ContextState.StorageBufferBound);
		//// glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ContextState.ElementArrayBufferBound);
		//// glBindBuffer( GL_ARRAY_BUFFER, ContextState.ArrayBufferBound);
		//
		//glActiveTexture(ContextState.ActiveTexture);
		ContextState.DepthMinZ = 0;
		ContextState.DepthMaxZ = 1;
		glDepthRangef(ContextState.DepthMinZ, ContextState.DepthMaxZ);
		ContextState.DepthStencilState.bZEnable = true;
		glEnable(GL_DEPTH_TEST);
		ContextState.DepthStencilState.bZWriteEnable = true;
		glDepthMask(ContextState.DepthStencilState.bZWriteEnable);
		ContextState.DepthStencilState.ZFunc = GL_LESS;
		glDepthFunc(ContextState.DepthStencilState.ZFunc);
		ContextState.DepthStencilState.bStencilEnable = true;
		glEnable(GL_STENCIL_TEST);
		ContextState.DepthStencilState.StencilWriteMask = true;
		glStencilMask(ContextState.DepthStencilState.StencilWriteMask);
		glClearDepthf(1);
		glClearStencil(0);
		
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DITHER);

		ContextState.DepthStencilState.bZEnable = false;
		glDisable(GL_DEPTH_TEST);
		ContextState.RasterizerState.DepthBias = 0;
		ContextState.RasterizerState.SlopeScaleDepthBias = 0;
		glDisable(GL_POLYGON_OFFSET_FILL);
		glDisable(GL_POLYGON_OFFSET_LINE);
		glDisable(GL_POLYGON_OFFSET_POINT);
		
		glDisable(GL_RASTERIZER_DISCARD);
		ContextState.bAlphaToCoverageEnabled = false;
		glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		glDisable(GL_SAMPLE_COVERAGE);
		ContextState.bScissorEnabled = false;
		glDisable(GL_SCISSOR_TEST);
		ContextState.DepthStencilState.bStencilEnable = false;
		glDisable(GL_STENCIL_TEST);
		// glDisable(GL_COLOR_LOGIC_OP);
		// glDisable(GL_INDEX_LOGIC_OP);
		// glDisable(GL_ALPHA_TEST);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glPixelStorei(GL_PACK_ROW_LENGTH, 0);
		glPixelStorei(GL_PACK_SKIP_ROWS, 0);
		glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
		glPixelStorei(GL_PACK_ALIGNMENT, 4);

		glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );

		// glBindVertexArray(0); //this call makes the screen black
		glBindVertexArray(AndroidEGL::GetInstance()->GetRenderingContext()->DefaultVertexArrayObject);
		ContextState.Framebuffer = 0;
		glBindFramebuffer(GL_FRAMEBUFFER, ContextState.Framebuffer);
		ContextState.ElementArrayBufferBound = 0;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ContextState.ElementArrayBufferBound);
		ContextState.ArrayBufferBound = 0;
		glBindBuffer(GL_ARRAY_BUFFER, ContextState.ArrayBufferBound);
		ContextState.UniformBufferBound = 0;
		glBindBuffer(GL_UNIFORM_BUFFER, ContextState.UniformBufferBound);
		ContextState.PixelUnpackBufferBound = 0;
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, ContextState.PixelUnpackBufferBound);

		//const IOpenGLDynamicRHI* OpenGlRHI = GetIOpenGLDynamicRHI();
		//GLuint OpenGLResourcePtr = (GLuint)OpenGlRHI->RHIGetResource(CheckerboardTexture->GetResource()->TextureRHI);
		
		for (int i = 0; i <= 7; i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, 0);
			ContextState.Textures[i].Resource = 0;
		}
		glActiveTexture(GL_TEXTURE0);
		ContextState.ActiveTexture = 0;
		glFrontFace(GL_CCW);
				
		// UE_LOG(LogRiveRenderer, Display, TEXT("%s OpenGLDynamicRHI->RHIInvalidateCachedState()"), FDebugLogger::Ind(), PLSRenderContextPtr);
		// OpenGLDynamicRHI->RHIInvalidateCachedState(); // lose the 3D ground
		OpenGLDynamicRHI->RHIPostExternalCommandsReset();
		// OpenGLDynamicRHI->RHIInvalidateCachedState(); // purely black ground

		RIVE_DEBUG_VERBOSE("Post RHIPostExternalCommandsReset");
	}
}

#endif // WITH_RIVE

#endif
