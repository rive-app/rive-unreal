// Copyright Rive, Inc. All rights reserved.

#include "Platform/RiveRenderTargetOpenGL.h"

#if PLATFORM_ANDROID
#include "OpenGLDrv.h"
#include "IRiveRendererModule.h"
#include "RiveRendererOpenGL.h"
#include "RiveRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Logs/RiveRendererLog.h"

#include "IOpenGLDynamicRHI.h"


#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/artboard.hpp"
#include "rive/pls/pls_renderer.hpp"
#include "rive/pls/gl/pls_render_context_gl_impl.hpp"
#include "rive/pls/gl/pls_render_target_gl.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::FRiveRenderTargetOpenGL(const TSharedRef<FRiveRendererOpenGL>& InRiveRenderer, const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget)
	: FRiveRenderTarget(InRiveRenderer, InRiveName, InRenderTarget), RiveRendererGL(InRiveRenderer)
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s === FRiveRenderTargetOpenGL::FRiveRenderTargetOpenGL"), FDebugLogger::Ind());
}

UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::~FRiveRenderTargetOpenGL()
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s ~~~ FRiveRenderTargetOpenGL::~FRiveRenderTargetOpenGL"), FDebugLogger::Ind());
}

void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::Initialize()
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRenderTargetOpenGL::Initialize"), FDebugLogger::Ind()); FScopeLogIndent LogIndent{};
	check(IsInGameThread());
	
	FScopeLock Lock(&ThreadDataCS);

	// FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();

	if (IRiveRendererModule::RunInGameThread())
	{
		if (!IRiveRendererModule::ReCacheTextureEveryFrame())
		{
			CacheTextureTarget_Internal(RenderTarget->GameThread_GetRenderTargetResource()->TextureRHI);
		}
	}
	else
	{
		ENQUEUE_RENDER_COMMAND(CacheTextureTarget_RenderThread)(
		[RenderTarget = RenderTarget, this](FRHICommandListImmediate& RHICmdList)
		// [RenderTargetResource, this](FRHICommandListImmediate& RHICmdList)
		{
			if (!IRiveRendererModule::ReCacheTextureEveryFrame())
			{
				CacheTextureTarget_RenderThread(RHICmdList, RenderTarget->GetResource()->TextureRHI);
			}
		});
	}
}

DECLARE_GPU_STAT_NAMED(CacheTextureTarget, TEXT("FRiveRenderTargetOpenGL::CacheTextureTarget_RenderThread"));
void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InTexture)
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRenderTargetOpenGL::CacheTextureTarget_RenderThread"), FDebugLogger::Ind()); FScopeLogIndent LogIndent{};
	
	check(IsInRenderingThread());
	RHICmdList.EnqueueLambda([this, InTexture](FRHICommandListImmediate& RHICmdList)
	{
		CacheTextureTarget_Internal(InTexture);
	});
}

#if WITH_RIVE

void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::AlignArtboard(uint8 InFit, float AlignX, float AlignY, rive::Artboard* InNativeArtboard, const FLinearColor DebugColor)
{
	check(IsInGameThread());

	FScopeLock Lock(&ThreadDataCS);

	// FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();

	if (IRiveRendererModule::RunInGameThread())
	{
		if (IRiveRendererModule::ReCacheTextureEveryFrame())
		{
			CacheTextureTarget_Internal(RenderTarget->GameThread_GetRenderTargetResource()->TextureRHI);
		}
		AlignArtboard_Internal(InFit, AlignX, AlignY, InNativeArtboard, DebugColor);
	}
	else
	{
		ENQUEUE_RENDER_COMMAND(AlignArtboard)(
		[this, RenderTarget = RenderTarget, InFit, AlignX, AlignY, InNativeArtboard, DebugColor](FRHICommandListImmediate& RHICmdList)
		{
			if (IRiveRendererModule::ReCacheTextureEveryFrame())
			{
				CacheTextureTarget_RenderThread(RHICmdList, RenderTarget->GetResource()->TextureRHI);
			}
			AlignArtboard_RenderThread(RHICmdList, InFit, AlignX, AlignY, InNativeArtboard, DebugColor);
		});
	}
}

void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::DrawArtboard(rive::Artboard* InNativeArtboard, const FLinearColor DebugColor)
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRenderTargetOpenGL::DrawArtboard -skipped"), FDebugLogger::Ind()); FScopeLogIndent LogIndent{};
	check(IsInGameThread());
}

DECLARE_GPU_STAT_NAMED(AlignArtboard, TEXT("FRiveRenderTargetOpenGL::AlignArtboard"));
void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::AlignArtboard_RenderThread(FRHICommandListImmediate& RHICmdList, uint8 InFit, float AlignX, float AlignY, rive::Artboard* InNativeArtboard, const FLinearColor DebugColor)
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRenderTargetOpenGL::AlignArtboard_RenderThread"), FDebugLogger::Ind()); FScopeLogIndent LogIndent{};
	SCOPED_GPU_STAT(RHICmdList, AlignArtboard);

	RHICmdList.EnqueueLambda([this, InFit, AlignX, AlignY, InNativeArtboard, DebugColor](FRHICommandListImmediate& RHICmdList)
	{
		AlignArtboard_Internal(InFit, AlignX, AlignY, InNativeArtboard, DebugColor);
	});
}

DECLARE_GPU_STAT_NAMED(DrawArtboard, TEXT("FRiveRenderTargetOpenGL::DrawArtboard"));

void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::AlignArtboard_Internal(uint8 InFit, float AlignX, float AlignY, rive::Artboard* InNativeArtboard, const FLinearColor DebugColor)
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRenderTargetOpenGL::AlignArtboard_Internal  [%s]"), FDebugLogger::Ind(), *FDebugLogger::CurrentThread()); FScopeLogIndent LogIndent{};
	check(IsInGameThread() || IsInRHIThread());
	ENABLE_VERIFY_GL_THREAD;
	
	FScopeLock Lock(&ThreadDataCS);
	rive::pls::PLSRenderContext* PLSRenderContextPtr = RiveRenderer->GetPLSRenderContextPtr();

	if (PLSRenderContextPtr == nullptr)
	{
		return;
	}
	// UE_LOG(LogRiveRenderer, Display, TEXT("%s PLSRenderContextPtr->resetGPUResources()  %p"), FDebugLogger::Ind(), PLSRenderContextPtr);
	// PLSRenderContextPtr->resetGPUResources();

	// Begin Frame
	std::unique_ptr<rive::pls::PLSRenderer> PLSRenderer = GetPLSRenderer_Internal(DebugColor);
	if (PLSRenderer == nullptr)
	{
		return;
	}
	
	const rive::Fit& Fit = *reinterpret_cast<rive::Fit*>(&InFit);
	const rive::Alignment& Alignment = rive::Alignment(AlignX, AlignY);
	const uint32 TextureWidth = GetWidth();
	const uint32 TextureHeight = GetHeight();
	rive::Mat2D Transform = rive::computeAlignment(
		Fit,
		Alignment,
		rive::AABB(0.f, 0.f, TextureWidth, TextureHeight),
		InNativeArtboard->bounds());
	
	PLSRenderer->save();
	// We need to invert the Y Axis for OpenGL
	PLSRenderer->transform(rive::Mat2D::fromScaleAndTranslation(1.f, -1.f, 0.f, TextureHeight) * Transform);
	UE_LOG(LogRiveRenderer, Display, TEXT("%s InNativeArtboard->draw()"), FDebugLogger::Ind());
	InNativeArtboard->draw(PLSRenderer.get());

	PLSRenderer->restore();
	
	{ // End drawing a frame.
		// Flush
		UE_LOG(LogRiveRenderer, Display, TEXT("%s PLSRenderContextPtr->flush()  %p"), FDebugLogger::Ind(), PLSRenderContextPtr);
		PLSRenderContextPtr->flush();

		TArray<FIntVector2> Points{{0,0}, {100,100}, {200,200}, {300,300}};
		for (FIntVector2 Point : Points) 
		{
			if (Point.X < TextureWidth && Point.Y < TextureHeight)
			{
				GLubyte pix[4];
				glReadPixels(Point.X, Point.Y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pix);
				UE_LOG(LogRiveRenderer, Display, TEXT("Pixel [%d,%d] = %u %u %u %u"), Point.X, Point.Y, pix[0], pix[1], pix[2], pix[3])
			}
		}
		
		UE_LOG(LogRiveRenderer, Warning, TEXT("%s PLSRenderContextGLImpl->resetGLState() %p"), FDebugLogger::Ind(), PLSRenderContextPtr);
		PLSRenderContextPtr->static_impl_cast<rive::pls::PLSRenderContextGLImpl>()->resetGLState();
		
		// Reset
		UE_LOG(LogRiveRenderer, Display, TEXT("%s PLSRenderContextPtr->shrinkGPUResourcesToFit() %p"), FDebugLogger::Ind(), PLSRenderContextPtr);
		PLSRenderContextPtr->shrinkGPUResourcesToFit();
		UE_LOG(LogRiveRenderer, Display, TEXT("%s PLSRenderContextPtr->resetGPUResources()  %p"), FDebugLogger::Ind(), PLSRenderContextPtr);
		PLSRenderContextPtr->resetGPUResources();
	}

	if (IsInRHIThread())
	{
		FOpenGLDynamicRHI* OpenGLDynamicRHI = static_cast<FOpenGLDynamicRHI*>(GetIOpenGLDynamicRHI());
		FOpenGLContextState& ContextState = OpenGLDynamicRHI->GetContextStateForCurrentContext(true);
			
		// UE_LOG(LogRiveRenderer, Display, TEXT("%s OpenGLDynamicRHI->RHIInvalidateCachedState()"), FDebugLogger::Ind(), PLSRenderContextPtr);
		// OpenGLDynamicRHI->RHIInvalidateCachedState();

		UE_LOG(LogRiveRenderer, Warning, TEXT("%s Pre RHIPostExternalCommandsReset"), FDebugLogger::Ind());
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
		
		UE_LOG(LogRiveRenderer, Warning, TEXT("%s Post RHIPostExternalCommandsReset"), FDebugLogger::Ind());
	}
}

std::unique_ptr<rive::pls::PLSRenderer> UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::GetPLSRenderer_Internal(const FLinearColor DebugColor) const
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRenderTargetOpenGL::GetPLSRenderer_Internal  [%s]"), FDebugLogger::Ind(), *FDebugLogger::CurrentThread()); FScopeLogIndent LogIndent{};
	check(IsInGameThread() || IsInRHIThread());
	ENABLE_VERIFY_GL_THREAD;
	
	rive::pls::PLSRenderContext* PLSRenderContextPtr = RiveRendererGL->GetPLSRenderContextPtr();

	if (PLSRenderContextPtr == nullptr || !FRiveRendererOpenGL::IsRHIOpenGL())
	{
		return nullptr;
	}
	
	FColor ClearColor = DebugColor.ToRGBE();
	
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s PLSRenderContextGLImpl->resetGLState() %p"), FDebugLogger::Ind(), PLSRenderContextPtr);
	PLSRenderContextPtr->static_impl_cast<rive::pls::PLSRenderContextGLImpl>()->resetGLState();
	rive::pls::PLSRenderContext::FrameDescriptor FrameDescriptor;

	UE_LOG(LogRiveRenderer, Display, TEXT("%s FRiveRenderTargetOpenGL CachedPLSRenderTargetOpenGL  %p"), FDebugLogger::Ind(), CachedPLSRenderTargetOpenGL.get());
	FrameDescriptor.renderTarget = CachedPLSRenderTargetOpenGL;
	FrameDescriptor.loadAction = rive::pls::LoadAction::clear; //bIsCleared ? rive::pls::LoadAction::clear : rive::pls::LoadAction::preserveRenderTarget;
	FrameDescriptor.clearColor = rive::colorARGB(ClearColor.A, ClearColor.R, ClearColor.G, ClearColor.B);
	FrameDescriptor.wireframe = false;
	FrameDescriptor.fillsDisabled = false;
	FrameDescriptor.strokesDisabled = false;

	if (bIsCleared == false)
	{
		bIsCleared = true;
	}
	
	UE_LOG(LogRiveRenderer, Display, TEXT("%s FRiveRenderTargetOpenGL PLSRenderContextPtr->beginFrame %p"), FDebugLogger::Ind(), PLSRenderContextPtr);
	PLSRenderContextPtr->beginFrame(std::move(FrameDescriptor));

	std::unique_ptr<rive::pls::PLSRenderer> Renderer = std::make_unique<rive::pls::PLSRenderer>(PLSRenderContextPtr);
	UE_LOG(LogRiveRenderer, Display, TEXT("%s FRiveRenderTargetOpenGL PLSRenderer: %p  from RenderContext: %p"), FDebugLogger::Ind(), Renderer.get(), PLSRenderContextPtr);
	return Renderer;
}

void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::CacheTextureTarget_Internal(const FTexture2DRHIRef& InRHIResource)
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRenderTargetOpenGL::CacheTextureTarget_Internal  [%s]"), FDebugLogger::Ind(), *FDebugLogger::CurrentThread()); FScopeLogIndent LogIndent{};
	check(IsInGameThread() || IsInRHIThread());
	ENABLE_VERIFY_GL_THREAD;

	if (!ensure(FRiveRendererOpenGL::IsRHIOpenGL()) || !InRHIResource.IsValid())
	{
		UE_LOG(LogRiveRenderer, Error, TEXT("%s either RHI, OpenGL or Texture is not valid."), FDebugLogger::Ind());
		UE_LOG(LogRiveRenderer, Display, TEXT("%s    Dynamic RHI is valid?  %s"), FDebugLogger::Ind(), GDynamicRHI ? TEXT("TRUE") : TEXT("FALSE"));
		UE_LOG(LogRiveRenderer, Display, TEXT("%s    Dynamic RHI is OpenGL?  %s"), FDebugLogger::Ind(), GDynamicRHI && GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::OpenGL ? TEXT("TRUE") : TEXT("FALSE"));
		UE_LOG(LogRiveRenderer, Display, TEXT("%s    InTexture is valid?  %s"), FDebugLogger::Ind(), InRHIResource.IsValid() ? TEXT("TRUE") : TEXT("FALSE"));
		return;
	}
	
	FScopeLock Lock(&ThreadDataCS);
#if WITH_RIVE
	rive::pls::PLSRenderContext* PLSRenderContext = RiveRendererGL->GetOrCreatePLSRenderContextPtr_Internal();

	if (PLSRenderContext == nullptr)
	{
		UE_LOG(LogRiveRenderer, Error, TEXT("%s PLSRenderContext == nullptr"), FDebugLogger::Ind());
		return;
	}
	UE_LOG(LogRiveRenderer, Display, TEXT("%s PLSRenderContext is valid"), FDebugLogger::Ind());
#endif // WITH_RIVE

	IOpenGLDynamicRHI* OpenGLRHI = GetIOpenGLDynamicRHI();
	GLuint OpenGLResourcePtr = (GLuint)OpenGLRHI->RHIGetResource(InRHIResource);
	UE_LOG(LogRiveRenderer, Display, TEXT("%s OpenGLResourcePtr  %d"), FDebugLogger::Ind(), OpenGLResourcePtr);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, OpenGLResourcePtr);
	GLint format;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	UE_LOG(LogRiveRenderer, Error, TEXT("%s GL_TEXTURE_INTERNAL_FORMAT: %x (d: %d)  is GL_RGBA8? %s"), FDebugLogger::Ind(), format, format, format == GL_RGBA8 ? TEXT("TRUE") : TEXT("FALSE"));
	
	if (format != GL_RGBA8)
	{
		FString PixelFormatString = GetPixelFormatString(InRHIResource->GetFormat());
		UE_LOG(LogRiveRenderer, Error, TEXT("%s Wrong Pixel Format (not GL_RGBA8 %x (d: %d)), PixelFormat is '%s'. GL_TEXTURE_INTERNAL_FORMAT: %x (d: %d)"), FDebugLogger::Ind(), GL_RGBA8, GL_RGBA8, *PixelFormatString, format, format);
		return;
	}

#if WITH_RIVE
	// For now we just set one renderer and one texture
	rive::pls::PLSRenderContextGLImpl* PLSRenderContextGLImpl = PLSRenderContext->static_impl_cast<rive::pls::PLSRenderContextGLImpl>();
	UE_LOG(LogRiveRenderer, Display, TEXT("%s PLSRenderContextGLImpl->resetGLState()"), FDebugLogger::Ind());
	PLSRenderContextGLImpl->resetGLState();

	int w, h;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, OpenGLResourcePtr);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

	if (w == 0 || h == 0)
	{
		UE_LOG(LogRiveRenderer, Error, TEXT("%s size is 0!   w: %d, h: %d"), FDebugLogger::Ind(), w, h);
		return;
	}
	if (!ensureMsgf(w == GetWidth() && h == GetHeight(), TEXT("Texture Size retrieved from OpenGL is not matching Render Target!  GL: %dx%d  RT: %dx%d"), w,h,GetWidth(), GetHeight()))
	{
		return;
	}
	UE_LOG(LogRiveRenderer, Display, TEXT("%s OpenGLResourcePtr %d texture size %dx%d"), FDebugLogger::Ind(), OpenGLResourcePtr, w, h);

	CachedPLSRenderTargetOpenGL = rive::make_rcp<rive::pls::TextureRenderTargetGL>(w, h);
	UE_LOG(LogRiveRenderer, Display, TEXT("%s PLSRenderContextGLImpl->setTargetTexture( %d )"), FDebugLogger::Ind(), OpenGLResourcePtr);
	CachedPLSRenderTargetOpenGL->setTargetTexture(OpenGLResourcePtr);
	UE_LOG(LogRiveRenderer, Display, TEXT("%s CachedPLSRenderTargetOpenGL set to  %p"), FDebugLogger::Ind(), CachedPLSRenderTargetOpenGL.get());
#endif // WITH_RIVE
}

#endif // WITH_RIVE

#endif
