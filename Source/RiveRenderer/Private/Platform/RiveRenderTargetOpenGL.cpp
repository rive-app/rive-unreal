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
	TArray<FIntVector2> Points{ {0,0}, { 100,100 }, { 200,200 }, { 300,300 }}; 
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
	PLSRenderContextPtr->static_impl_cast<rive::pls::PLSRenderContextGLImpl>()->unbindGLInternalResources();

	if (IsInRHIThread()) //todo: still not working, to be looked at
	{
		FOpenGLDynamicRHI* OpenGLDynamicRHI = static_cast<FOpenGLDynamicRHI*>(GetIOpenGLDynamicRHI());
		FOpenGLContextState& ContextState = OpenGLDynamicRHI->GetContextStateForCurrentContext(true);
			
		// UE_LOG(LogRiveRenderer, Display, TEXT("%s OpenGLDynamicRHI->RHIInvalidateCachedState()"), FDebugLogger::Ind(), PLSRenderContextPtr);
		// OpenGLDynamicRHI->RHIInvalidateCachedState();

		RIVE_DEBUG_VERBOSE("Pre RHIPostExternalCommandsReset");
		// Manual reset of GL commands to match the GL State before Rive Commands. Supposed to be handled by RHIPostExternalCommandsReset, TBC
		FOpenGL::BindProgramPipeline(ContextState.Program);
		glViewport(ContextState.Viewport.Min.X, ContextState.Viewport.Min.Y, ContextState.Viewport.Max.X - ContextState.Viewport.Min.X, ContextState.Viewport.Max.Y - ContextState.Viewport.Min.Y);
		FOpenGL::DepthRange(ContextState.DepthMinZ, ContextState.DepthMaxZ);
		ContextState.bScissorEnabled ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);
		glScissor(ContextState.Scissor.Min.X, ContextState.Scissor.Min.Y, ContextState.Scissor.Max.X - ContextState.Scissor.Min.X, ContextState.Scissor.Max.Y - ContextState.Scissor.Min.Y);
		glBindFramebuffer(GL_FRAMEBUFFER, ContextState.Framebuffer);
			
		FOpenGL::PolygonMode(GL_FRONT_AND_BACK, ContextState.RasterizerState.FillMode);
		glCullFace(ContextState.RasterizerState.CullMode);
		glBindBuffer( GL_PIXEL_UNPACK_BUFFER, ContextState.PixelUnpackBufferBound);
		glBindBuffer( GL_UNIFORM_BUFFER, ContextState.UniformBufferBound);
		glBindBuffer( GL_SHADER_STORAGE_BUFFER, ContextState.StorageBufferBound);
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ContextState.ElementArrayBufferBound);
		glBindBuffer( GL_ARRAY_BUFFER, ContextState.ArrayBufferBound);
			
		for (int i = 0; i < ContextState.Textures.Num(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, ContextState.Textures[i].Resource);
		}
		glActiveTexture(ContextState.ActiveTexture);
		glFrontFace(GL_CCW);
		
		OpenGLDynamicRHI->RHIPostExternalCommandsReset();
		
		RIVE_DEBUG_VERBOSE("Post RHIPostExternalCommandsReset");
	}
}

void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::Render_RenderThread(FRHICommandListImmediate& RHICmdList, const TArray<FRiveRenderCommand>& RiveRenderCommands)
{
	RIVE_DEBUG_FUNCTION_INDENT;
	check(IsInRenderingThread());
	
	RHICmdList.EnqueueLambda([this, RiveRenderCommands = RiveRenderCommands](FRHICommandListImmediate& RHICmdList)
	{
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
#endif // WITH_RIVE
}

#endif // WITH_RIVE

#endif
