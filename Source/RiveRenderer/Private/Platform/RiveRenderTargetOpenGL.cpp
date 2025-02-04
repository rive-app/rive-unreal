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
THIRD_PARTY_INCLUDES_START
#include "rive/artboard.hpp"
#include "rive/renderer.hpp"
#include "rive/renderer/gl/render_context_gl_impl.hpp"
#include "rive/renderer/gl/render_target_gl.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

FRiveRenderTargetOpenGL::FRiveRenderTargetOpenGL(
    const TSharedRef<FRiveRendererOpenGL>& InRiveRenderer,
    const FName& InRiveName,
    UTexture2DDynamic* InRenderTarget) :
    FRiveRenderTarget(InRiveRenderer, InRiveName, InRenderTarget),
    RiveRendererGL(InRiveRenderer)
{
    RIVE_DEBUG_FUNCTION_INDENT;
}

FRiveRenderTargetOpenGL::~FRiveRenderTargetOpenGL()
{
    RIVE_DEBUG_FUNCTION_INDENT;
    RenderTargetOpenGL.reset();
}

void FRiveRenderTargetOpenGL::Initialize()
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
        ENQUEUE_RENDER_COMMAND(CacheTextureTarget_RenderThread)
        ([RenderTargetResource, this](FRHICommandListImmediate& RHICmdList) {
            CacheTextureTarget_RenderThread(RHICmdList,
                                            RenderTargetResource->TextureRHI);
        });
    }
}

DECLARE_GPU_STAT_NAMED(
    CacheTextureTarget,
    TEXT("FRiveRenderTargetOpenGL::CacheTextureTarget_RenderThread"));
void FRiveRenderTargetOpenGL::CacheTextureTarget_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    const FTextureRHIRef& InTexture)
{
    RIVE_DEBUG_FUNCTION_INDENT;
    check(IsInRenderingThread());

    if (IRiveRendererModule::RunInGameThread())
    {
        AsyncTask(ENamedThreads::GameThread, [this, InTexture]() {
            CacheTextureTarget_Internal(InTexture);
        });
    }
    else
    {
        SCOPED_GPU_STAT(RHICmdList, CacheTextureTarget);
        RHICmdList.EnqueueLambda(
            [this, InTexture](FRHICommandListImmediate& RHICmdList) {
                CacheTextureTarget_Internal(InTexture);
            });
    }
}

void FRiveRenderTargetOpenGL::Submit()
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

rive::rcp<rive::gpu::RenderTarget> FRiveRenderTargetOpenGL::GetRenderTarget()
    const
{
    return RenderTargetOpenGL;
}

std::unique_ptr<rive::RiveRenderer> FRiveRenderTargetOpenGL::BeginFrame()
{
    RIVE_DEBUG_FUNCTION_INDENT;
    check(IsInGameThread() || IsInRHIThread());
    ENABLE_VERIFY_GL_THREAD;

    rive::gpu::RenderContext* RenderContext = RiveRenderer->GetRenderContext();
    if (RenderContext == nullptr)
    {
        return nullptr;
    }

    RIVE_DEBUG_VERBOSE("RenderContextGLImpl->invalidateGLState() %p",
                       RenderContext);
    RenderContext->static_impl_cast<rive::gpu::RenderContextGLImpl>()
        ->invalidateGLState();

    return FRiveRenderTarget::BeginFrame();
}

void FRiveRenderTargetOpenGL::EndFrame() const
{
    RIVE_DEBUG_FUNCTION_INDENT;
    check(IsInGameThread() || IsInRHIThread());
    ENABLE_VERIFY_GL_THREAD;

    rive::gpu::RenderContext* RenderContext = RiveRenderer->GetRenderContext();
    if (RenderContext == nullptr)
    {
        return;
    }

    // End drawing a frame.
    // Flush
    RIVE_DEBUG_VERBOSE("RenderContext->flush()  %p", RenderContext);
    const rive::gpu::RenderContext::FlushResources FlushResources{
        GetRenderTarget().get()};
    RenderContext->flush(FlushResources);

    // todo: to remove once OpenGL fully fixed
    TArray<FIntVector2> Points{{0, 0},
                               {100, 100},
                               {200, 200},
                               {300, 300},
                               {400, 400},
                               {500, 500},
                               {600, 600},
                               {700, 700}};
    for (FIntVector2 Point : Points)
    {
        if (Point.X < GetWidth() && Point.Y < GetHeight())
        {
            GLubyte pix[4];
            glReadPixels(Point.X,
                         Point.Y,
                         1,
                         1,
                         GL_RGBA,
                         GL_UNSIGNED_BYTE,
                         &pix);
            RIVE_DEBUG_VERBOSE("Pixel [%d,%d] = %u %u %u %u",
                               Point.X,
                               Point.Y,
                               pix[0],
                               pix[1],
                               pix[2],
                               pix[3])
        }
    }

    // Reset
    RIVE_DEBUG_VERBOSE("RenderContext->unbindGLInternalResources() %p",
                       RenderContext);
    RenderContext->static_impl_cast<rive::gpu::RenderContextGLImpl>()
        ->unbindGLInternalResources(); // careful, need UE's internal state to
                                       // match

    ResetOpenGLState();
}

void FRiveRenderTargetOpenGL::Render_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    const TArray<FRiveRenderCommand>& RiveRenderCommands)
{
    RIVE_DEBUG_FUNCTION_INDENT;
    check(IsInRenderingThread());

    RHICmdList.EnqueueLambda([this, RiveRenderCommands = RiveRenderCommands](
                                 FRHICommandListImmediate& RHICmdList) {
        Render_Internal(RiveRenderCommands);
    });
}

#if WITH_RIVE

void FRiveRenderTargetOpenGL::CacheTextureTarget_Internal(
    const FTextureRHIRef& InRHIResource)
{
    RIVE_DEBUG_FUNCTION_INDENT;
    check(IsInGameThread() || IsInRHIThread());
    ENABLE_VERIFY_GL_THREAD;

    if (!ensure(FRiveRendererOpenGL::IsRHIOpenGL()) || !InRHIResource.IsValid())
    {
        UE_LOG(LogRiveRenderer,
               Error,
               TEXT("Error while caching the TextureTarget: RHI, OpenGL or "
                    "Texture is not valid."));
        return;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

#if WITH_RIVE
    rive::gpu::RenderContext* RenderContext =
        RiveRendererGL->GetOrCreateRenderContext_Internal();

    if (RenderContext == nullptr)
    {
        UE_LOG(LogRiveRenderer, Error, TEXT("RenderContext is null"));
        return;
    }
    RIVE_DEBUG_VERBOSE("RenderContext is valid");
#endif // WITH_RIVE

    const IOpenGLDynamicRHI* OpenGlRHI = GetIOpenGLDynamicRHI();
    GLuint OpenGLResourcePtr = (GLuint)OpenGlRHI->RHIGetResource(InRHIResource);
    RIVE_DEBUG_VERBOSE("OpenGLResourcePtr  %d", OpenGLResourcePtr);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, OpenGLResourcePtr);
    GLint format;
    glGetTexLevelParameteriv(GL_TEXTURE_2D,
                             0,
                             GL_TEXTURE_INTERNAL_FORMAT,
                             &format);
    glBindTexture(GL_TEXTURE_2D, 0);

    RIVE_DEBUG_VERBOSE(
        "GL_TEXTURE_INTERNAL_FORMAT: %x (d: %d)  is GL_RGBA8? %s",
        format,
        format,
        format == GL_RGBA8 ? TEXT("TRUE") : TEXT("FALSE"));

    if (format != GL_RGBA8)
    {
        const FString PixelFormatString =
            GetPixelFormatString(InRHIResource->GetFormat());
        UE_LOG(
            LogRiveRenderer,
            Error,
            TEXT("Texture Target has the wrong Pixel Format (not GL_RGBA8 %x "
                 "(d: %d)), "
                 "PixelFormat is '%s'. GL_TEXTURE_INTERNAL_FORMAT: %x (d: %d)"),
            GL_RGBA8,
            GL_RGBA8,
            *PixelFormatString,
            format,
            format);
        return;
    }

#if WITH_RIVE
    // For now we just set one renderer and one texture
    rive::gpu::RenderContextGLImpl* RenderContextGLImpl =
        RenderContext->static_impl_cast<rive::gpu::RenderContextGLImpl>();
    RIVE_DEBUG_VERBOSE("RenderContextGLImpl->resetGLState()");
    RenderContextGLImpl->invalidateGLState();

    int w, h;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, OpenGLResourcePtr);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

    if (w == 0 || h == 0)
    {
        UE_LOG(LogRiveRenderer,
               Error,
               TEXT("The size of the Texture Target is 0!   w: %d, h: %d"),
               w,
               h);
        return;
    }
    RIVE_DEBUG_VERBOSE("OpenGLResourcePtr %d texture size %dx%d",
                       OpenGLResourcePtr,
                       w,
                       h);

    if (RenderTargetOpenGL)
    {
        RenderTargetOpenGL.reset();
    }

    RenderTargetOpenGL = rive::make_rcp<rive::gpu::TextureRenderTargetGL>(w, h);
    RIVE_DEBUG_VERBOSE("RenderContextGLImpl->setTargetTexture( %d )",
                       OpenGLResourcePtr);
    RenderTargetOpenGL->setTargetTexture(OpenGLResourcePtr);
    RIVE_DEBUG_VERBOSE("RenderTargetOpenGL set to  %p",
                       RenderTargetOpenGL.get());

    ResetOpenGLState();
#endif // WITH_RIVE
}

void FRiveRenderTargetOpenGL::ResetOpenGLState() const
{
    if (!IsInRHIThread())
    {
        return;
    }
    // todo: still not fully working, to be looked at
    FOpenGLDynamicRHI* OpenGLDynamicRHI =
        static_cast<FOpenGLDynamicRHI*>(GetIOpenGLDynamicRHI());
    FOpenGLContextState& ContextState =
        OpenGLDynamicRHI->GetContextStateForCurrentContext(true);

    RIVE_DEBUG_VERBOSE("Pre RHIPostExternalCommandsReset");

    // Here, we manually set the context values as per the changes that have
    // been made by Rive. We call the GL function to be sure
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
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    glBindVertexArray(AndroidEGL::GetInstance()
                          ->GetRenderingContext()
                          ->DefaultVertexArrayObject);
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

    for (int i = 0; i <= 7; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
        ContextState.Textures[i].Resource = 0;
    }
    glActiveTexture(GL_TEXTURE0);
    ContextState.ActiveTexture = 0;
    glFrontFace(GL_CCW);

    OpenGLDynamicRHI->RHIPostExternalCommandsReset();
    RIVE_DEBUG_VERBOSE("Post RHIPostExternalCommandsReset");
}

#endif // WITH_RIVE

#endif
