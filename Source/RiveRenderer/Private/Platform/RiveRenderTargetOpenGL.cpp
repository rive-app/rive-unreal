// Copyright Rive, Inc. All rights reserved.

#include "Platform/RiveRenderTargetOpenGL.h"

// #include "GLES2/gl2ext.h"
// #include "OpenGLDrv/Private/Android/AndroidOpenGL.h"
#include "OpenGLDrv.h"

#include "IRiveRendererModule.h"
#include "RiveRendererOpenGL.h"

// #undef PLATFORM_ANDROID
// #define PLATFORM_ANDROID 1
#if PLATFORM_ANDROID
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
// #include <GL/glcorearb.h>
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

UE_DISABLE_OPTIMIZATION

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
			CacheTextureTarget_GameThread(RenderTarget->GameThread_GetRenderTargetResource()->TextureRHI);
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
		CacheTextureTarget_RHIThread(RHICmdList, InTexture);
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
			CacheTextureTarget_GameThread(RenderTarget->GameThread_GetRenderTargetResource()->TextureRHI);
		}
		AlignArtboard_GameThread(InFit, AlignX, AlignY, InNativeArtboard, DebugColor);
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
		AlignArtboard_RHIThread(RHICmdList, InFit, AlignX, AlignY, InNativeArtboard, DebugColor);
	});
}

void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::DrawArtboard_RenderThread(FRHICommandListImmediate& RHICmdList, rive::Artboard* InNativeArtboard, const FLinearColor DebugColor)
{
}

DECLARE_GPU_STAT_NAMED(DrawArtboard, TEXT("FRiveRenderTargetOpenGL::DrawArtboard"));

void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::CacheTextureTarget_RHIThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource)
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRenderTargetOpenGL::CacheTextureTarget_RHIThread"), FDebugLogger::Ind()); FScopeLogIndent LogIndent{};
	check(IsInRHIThread());

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
	rive::pls::PLSRenderContext* PLSRenderContext = RiveRendererGL->GetOrCreatePLSRenderContextPtr_RHIThread(RHICmdList);

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
		UE_LOG(LogRiveRenderer, Error, TEXT("%s Wrong Pixel Format (not GL_RGBA8), PixelFormat is '%s'. GL_TEXTURE_INTERNAL_FORMAT: %x (d: %d)"), FDebugLogger::Ind(), *PixelFormatString, format, format);
		return;
	}
	// SCOPED_GPU_STAT(RHICmdList, CacheTextureTarget);

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
	// RHICmdList.GetContext().RHIPostExternalCommandsReset();
	// RHICmdList.AllocCommand<FRHICommandPostExternalCommandsReset>();
	// FRHICommandPostExternalCommandsReset().Execute(RHICmdList);
	// static_cast<FOpenGLDynamicRHI*>(GetIOpenGLDynamicRHI())->RHIPostExternalCommandsReset();
#endif // WITH_RIVE
}

void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::AlignArtboard_GameThread(uint8 InFit, float AlignX, float AlignY, rive::Artboard* InNativeArtboard, const FLinearColor DebugColor)
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRenderTargetOpenGL::AlignArtboard_GameThread"), FDebugLogger::Ind()); FScopeLogIndent LogIndent{};
	check(IsInGameThread());
	FScopeLock Lock(&ThreadDataCS);
	rive::pls::PLSRenderContext* PLSRenderContextPtr = RiveRenderer->GetPLSRenderContextPtr();

	if (PLSRenderContextPtr == nullptr)
	{
		return;
	}
	UE_LOG(LogRiveRenderer, Display, TEXT("%s PLSRenderContextPtr->resetGPUResources()  %p"), FDebugLogger::Ind(), PLSRenderContextPtr);
	PLSRenderContextPtr->resetGPUResources();

	// Begin Frame
	std::unique_ptr<rive::pls::PLSRenderer> PLSRenderer = GetPLSRenderer_GameThread(DebugColor);
	if (PLSRenderer == nullptr)
	{
		return;
	}
	
	const rive::Fit& Fit = *reinterpret_cast<rive::Fit*>(&InFit);
	const rive::Alignment& Alignment = rive::Alignment(AlignX, AlignY);
	const uint32 TextureWidth = GetWidth();
	const uint32 TextureHeight = GetHeight();
	UE_LOG(LogRiveRenderer, Display, TEXT("%s computeAlignment()"), FDebugLogger::Ind());
	rive::Mat2D Transform = rive::computeAlignment(
		Fit,
		Alignment,
		rive::AABB(0.f, 0.f, TextureWidth, TextureHeight),
		InNativeArtboard->bounds());

	UE_LOG(LogRiveRenderer, Display, TEXT("%s PLSRenderer->save() %p"), FDebugLogger::Ind(), PLSRenderer.get());
	PLSRenderer->save();
	UE_LOG(LogRiveRenderer, Display, TEXT("%s PLSRenderer->transform() %p"), FDebugLogger::Ind(), PLSRenderer.get());
	PLSRenderer->transform(Transform);
	// EndFrame();
	UE_LOG(LogRiveRenderer, Display, TEXT("%s InNativeArtboard->draw() - skipped"), FDebugLogger::Ind());
	InNativeArtboard->draw(PLSRenderer.get());

	PLSRenderer->restore();
	
	{ // End drawing a frame.
		// Flush
		UE_LOG(LogRiveRenderer, Display, TEXT("%s PLSRenderContextPtr->flush()  %p"), FDebugLogger::Ind(), PLSRenderContextPtr);
		PLSRenderContextPtr->flush();

		rive::pls::PLSRenderContextGLImpl* PLSRenderContextGLImpl = PLSRenderContextPtr->static_impl_cast<rive::pls::PLSRenderContextGLImpl>();
		UE_LOG(LogRiveRenderer, Warning, TEXT("%s PLSRenderContextGLImpl->resetGLState() %p"), FDebugLogger::Ind(), PLSRenderContextGLImpl);
		PLSRenderContextGLImpl->resetGLState();
		glBindVertexArray(0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		
		for (int i = 0; i <= 7; i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		// Reset
		// UE_LOG(LogRiveRenderer, Display, TEXT("%s PLSRenderContextPtr->shrinkGPUResourcesToFit()"), FDebugLogger::Ind());
		// PLSRenderContextPtr->shrinkGPUResourcesToFit();
	}
	// RHICmdList.GetContext().RHIPostExternalCommandsReset();
}

void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::AlignArtboard_RHIThread(FRHICommandListImmediate& RHICmdList, uint8 InFit, float AlignX, float AlignY, rive::Artboard* InNativeArtboard, const FLinearColor DebugColor)
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRenderTargetOpenGL::AlignArtboard_RHIThread"), FDebugLogger::Ind()); FScopeLogIndent LogIndent{};
	check(IsInRHIThread());
	FScopeLock Lock(&ThreadDataCS);
	rive::pls::PLSRenderContext* PLSRenderContextPtr = RiveRenderer->GetPLSRenderContextPtr();

	if (PLSRenderContextPtr == nullptr)
	{
		return;
	}
	//UE_LOG(LogRiveRenderer, Display, TEXT("%s PLSRenderContextPtr->resetGPUResources()  %p"), FDebugLogger::Ind(), PLSRenderContextPtr);
	//PLSRenderContextPtr->resetGPUResources();
	IOpenGLDynamicRHI* OpenGLRHI = GetIOpenGLDynamicRHI();
	FOpenGLDynamicRHI* FOpenGLRHI = static_cast<FOpenGLDynamicRHI*>(GetIOpenGLDynamicRHI());

	// Begin Frame
	std::unique_ptr<rive::pls::PLSRenderer> PLSRenderer = GetPLSRenderer_RHIThread(DebugColor);
	if (PLSRenderer == nullptr)
	{
		return;
	}
	
	 const rive::Fit& Fit = *reinterpret_cast<rive::Fit*>(&InFit);
	 const rive::Alignment& Alignment = rive::Alignment(AlignX, AlignY);
	 const uint32 TextureWidth = GetWidth();
	 const uint32 TextureHeight = GetHeight();
	 UE_LOG(LogRiveRenderer, Display, TEXT("%s computeAlignment()"), FDebugLogger::Ind());
	 rive::Mat2D Transform = rive::computeAlignment(
	 	Fit,
	 	Alignment,
	 	rive::AABB(0.f, 0.f, TextureWidth, TextureHeight),
	 	InNativeArtboard->bounds());

	UE_LOG(LogRiveRenderer, Display, TEXT("%s PLSRenderer->save() %p"), FDebugLogger::Ind(), PLSRenderer.get());
	 PLSRenderer->save();
	UE_LOG(LogRiveRenderer, Display, TEXT("%s PLSRenderer->transform() %p"), FDebugLogger::Ind(), PLSRenderer.get());
	 PLSRenderer->transform(Transform);
	
	UE_LOG(LogRiveRenderer, Display, TEXT("%s InNativeArtboard->draw()"), FDebugLogger::Ind());
	 InNativeArtboard->draw(PLSRenderer.get());

	 PLSRenderer->restore();
	
	{ // End drawing a frame.
		// Flush
		UE_LOG(LogRiveRenderer, Display, TEXT("%s PLSRenderContextPtr->flush()  %p"), FDebugLogger::Ind(), PLSRenderContextPtr);
		PLSRenderContextPtr->flush();

		// UE_LOG(LogRiveRenderer, Warning, TEXT("%s PLSRenderContextGLImpl->resetGPUResources() %p"), FDebugLogger::Ind(), PLSRenderContextPtr);
		// PLSRenderContextPtr->resetGPUResources();

		int cursor_x = 0;
		int cursor_y = 0;
		// {
		// 	uint32_t *pixels = (uint32_t *)malloc(sizeof(uint32_t) * GetWidth() * GetHeight());
		// 	glReadPixels(0, 0, GetWidth(), GetHeight(), GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		// 	// GLubyte pixel = pixels[cursor_y * GetWidth() + cursor_x];
		// 	UE_LOG(LogRiveRenderer, Error, TEXT("Pixel [%d,%d] = %x"), cursor_x, cursor_y, pixels[0])
		// 	free(pixels);
		// }
		{
			GLubyte pix[4];
			glReadPixels(cursor_x, cursor_y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pix);
			UE_LOG(LogRiveRenderer, Error, TEXT("Pixel [%d,%d] = %u %u %u %u"), cursor_x, cursor_y, pix[0], pix[1], pix[2], pix[3])
		}
		
		rive::pls::PLSRenderContextGLImpl* PLSRenderContextGLImpl = PLSRenderContextPtr->static_impl_cast<rive::pls::PLSRenderContextGLImpl>();
		UE_LOG(LogRiveRenderer, Warning, TEXT("%s PLSRenderContextGLImpl->resetGLState() %p"), FDebugLogger::Ind(), PLSRenderContextGLImpl);
		PLSRenderContextGLImpl->resetGLState();
		// glBindVertexArray(0);
		// glBindFramebuffer(GL_FRAMEBUFFER, 0);
		// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		// glBindBuffer(GL_ARRAY_BUFFER, 0);
		// glBindBuffer(GL_UNIFORM_BUFFER, 0);
		// glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		//
		// for (int i = 0; i <= 7; i++)
		// {
		// 	glActiveTexture(GL_TEXTURE0 + i);
		// 	glBindTexture(GL_TEXTURE_2D, 0);
		// }
	}
	
	glFrontFace(GL_CCW);
	// glCullFace(GL_BACK);
	// static_cast<FOpenGLDynamicRHI*>(GetIOpenGLDynamicRHI())->RHIPostExternalCommandsReset();
	// IRHICommandContext& CmdListContext = RHICmdList.GetContext();
	// FRHICommandPostExternalCommandsReset().Execute(RHICmdList);
	// RHICmdList.AllocCommand<FRHICommandPostExternalCommandsReset>();
	RHICmdList.GetContext().RHIPostExternalCommandsReset();
}

std::unique_ptr<rive::pls::PLSRenderer> UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::GetPLSRenderer_GameThread(const FLinearColor DebugColor) const
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRenderTargetOpenGL::GetPLSRenderer_GameThread"), FDebugLogger::Ind()); FScopeLogIndent LogIndent{};
	check(IsInGameThread());
	rive::pls::PLSRenderContext* PLSRenderContextPtr = RiveRendererGL->GetPLSRenderContextPtr();

	if (PLSRenderContextPtr == nullptr || !FRiveRendererOpenGL::IsRHIOpenGL())
	{
		return nullptr;
	}
	
	FColor ClearColor = DebugColor.ToRGBE();
	ClearColor = FColor(0,255,0,255);
	
	rive::pls::PLSRenderContextGLImpl* PLSRenderContextGLImpl = PLSRenderContextPtr->static_impl_cast<rive::pls::PLSRenderContextGLImpl>();
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s PLSRenderContextGLImpl->resetGLState() %p"), FDebugLogger::Ind(), PLSRenderContextGLImpl);
	PLSRenderContextGLImpl->resetGLState();
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
	
	UE_LOG(LogRiveRenderer, Display, TEXT("%s FRiveRenderTargetOpenGL PLSRenderContextPtr->beginFrame %p"), FDebugLogger::Ind(), PLSRenderContextGLImpl);
	PLSRenderContextPtr->beginFrame(std::move(FrameDescriptor));

	std::unique_ptr<rive::pls::PLSRenderer> Renderer = std::make_unique<rive::pls::PLSRenderer>(PLSRenderContextPtr);
	UE_LOG(LogRiveRenderer, Display, TEXT("%s FRiveRenderTargetOpenGL PLSRenderer: %p  from RenderContext: %p"), FDebugLogger::Ind(), Renderer.get(), PLSRenderContextPtr);
	return Renderer;
}

std::unique_ptr<rive::pls::PLSRenderer> UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::GetPLSRenderer_RHIThread(const FLinearColor DebugColor) const
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRenderTargetOpenGL::GetPLSRenderer_RHIThread"), FDebugLogger::Ind()); FScopeLogIndent LogIndent{};
	check(IsInRHIThread());
	rive::pls::PLSRenderContext* PLSRenderContextPtr = RiveRendererGL->GetPLSRenderContextPtr();

	if (PLSRenderContextPtr == nullptr || !FRiveRendererOpenGL::IsRHIOpenGL())
	{
		return nullptr;
	}
	
	FColor ClearColor = DebugColor.ToRGBE();
	ClearColor = FColor(0,255,0,255);
	
	rive::pls::PLSRenderContextGLImpl* PLSRenderContextGLImpl = PLSRenderContextPtr->static_impl_cast<rive::pls::PLSRenderContextGLImpl>();
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s PLSRenderContextGLImpl->resetGLState() %p"), FDebugLogger::Ind(), PLSRenderContextGLImpl);
	PLSRenderContextGLImpl->resetGLState();
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
	
	UE_LOG(LogRiveRenderer, Display, TEXT("%s FRiveRenderTargetOpenGL PLSRenderContextPtr->beginFrame %p"), FDebugLogger::Ind(), PLSRenderContextGLImpl);
	PLSRenderContextPtr->beginFrame(std::move(FrameDescriptor));

	std::unique_ptr<rive::pls::PLSRenderer> Renderer = std::make_unique<rive::pls::PLSRenderer>(PLSRenderContextPtr);
	UE_LOG(LogRiveRenderer, Display, TEXT("%s FRiveRenderTargetOpenGL PLSRenderer: %p  from RenderContext: %p"), FDebugLogger::Ind(), Renderer.get(), PLSRenderContextPtr);
	return Renderer;
}

void UE::Rive::Renderer::Private::FRiveRenderTargetOpenGL::CacheTextureTarget_GameThread(const FTexture2DRHIRef& InRHIResource)
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRenderTargetOpenGL::CacheTextureTarget_GameThread"), FDebugLogger::Ind()); FScopeLogIndent LogIndent{};
	check(IsInGameThread());

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
	rive::pls::PLSRenderContext* PLSRenderContext = RiveRendererGL->GetOrCreatePLSRenderContextPtr_GameThread();

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
	// FRHICommandListImmediate::Get().GetContext().RHIPostExternalCommandsReset();
#endif // WITH_RIVE
}

#endif // WITH_RIVE

UE_ENABLE_OPTIMIZATION

#endif
