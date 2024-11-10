// Copyright Rive, Inc. All rights reserved.

#include "RiveRendererOpenGL.h"

#if PLATFORM_ANDROID
#include "RiveRenderTargetOpenGL.h"

#include "Async/Async.h"
#include "DynamicRHI.h"
#include "IRiveRendererModule.h"
#include "Logs/RiveRendererLog.h"
#include "OpenGLDrv.h"
#include "ProfilingDebugging/RealtimeGPUProfiler.h"
#include "TextureResource.h"
#include "UObject/UObjectGlobals.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/renderer/gl/render_context_gl_impl.hpp"
#include "rive/renderer.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

// ---------------------------------------------------
// --------------- FRiveRendererOpenGL ---------------
// ---------------------------------------------------

void FRiveRendererOpenGL::Initialize()
{
    check(IsInGameThread());
    RIVE_DEBUG_FUNCTION_INDENT

    {
        FString glVersionStr =
            ANSI_TO_TCHAR((const ANSICHAR*)glGetString(GL_VERSION));
        RIVE_DEBUG_VERBOSE("GAMETHREAD: GL_VERSION %s", *glVersionStr);

        ENQUEUE_RENDER_COMMAND(URiveFileInitialize_RenderThread)
        ([](FRHICommandListImmediate& RHICmdList) {
            FString glVersionStr =
                ANSI_TO_TCHAR((const ANSICHAR*)glGetString(GL_VERSION));
            RIVE_DEBUG_VERBOSE("RENDERTHREAD: GL_VERSION %s", *glVersionStr);
            RHICmdList.EnqueueLambda([](FRHICommandListImmediate&) {
                FString glVersionStr =
                    ANSI_TO_TCHAR((const ANSICHAR*)glGetString(GL_VERSION));
                RIVE_DEBUG_VERBOSE("RHITHREAD: GL_VERSION %s", *glVersionStr);
                FString glVendorStr =
                    ANSI_TO_TCHAR((const ANSICHAR*)glGetString(GL_VENDOR));
                RIVE_DEBUG_VERBOSE("RHITHREAD: GL_VENDOR %s", *glVendorStr);
                FString glRendererStr =
                    ANSI_TO_TCHAR((const ANSICHAR*)glGetString(GL_RENDERER));
                RIVE_DEBUG_VERBOSE("RHITHREAD: GL_RENDERER %s", *glRendererStr);
            });
        });
    }

    if (IRiveRendererModule::RunInGameThread())
    {
        {
            FScopeLock Lock(&ThreadDataCS);
            if (InitializationState != ERiveInitState::Uninitialized)
            {
                return;
            }
            InitializationState = ERiveInitState::Initializing;

            CreateRenderContext_GameThread();
            InitializationState = ERiveInitState::Initialized;
        }

        OnInitializedDelegate.Broadcast(this);
    }
    else
    {
        FRiveRenderer::Initialize();
    }
}

rive::gpu::RenderContext* FRiveRendererOpenGL::GetRenderContext()
{
    RIVE_DEBUG_FUNCTION_INDENT;
    if (ensure(IsRHIOpenGL()))
    {
        FScopeLock Lock(&ContextsCS);
        if (RenderContext)
        {
            return RenderContext.get();
        }
    }

    UE_LOG(LogRiveRenderer,
           Error,
           TEXT("Rive Render Context is uninitialized for the Current OpenGL "
                "Context."));
    return nullptr;
}

TSharedPtr<IRiveRenderTarget> FRiveRendererOpenGL::
    CreateTextureTarget_GameThread(const FName& InRiveName,
                                   UTexture2DDynamic* InRenderTarget)
{
    RIVE_DEBUG_FUNCTION_INDENT;
    check(IsInGameThread());

    const TSharedPtr<FRiveRenderTargetOpenGL> RiveRenderTarget =
        MakeShared<FRiveRenderTargetOpenGL>(SharedThis(this),
                                            InRiveName,
                                            InRenderTarget);
    RenderTargets.Add(InRiveName, RiveRenderTarget);

    return RiveRenderTarget;
}

DECLARE_GPU_STAT_NAMED(CreateRenderContext,
                       TEXT("CreateRenderContext_RenderThread"));
void FRiveRendererOpenGL::CreateRenderContext_RenderThread(
    FRHICommandListImmediate& RHICmdList)
{
    RIVE_DEBUG_FUNCTION_INDENT;
    check(IsInRenderingThread());

    RHICmdList.EnqueueLambda(
        GET_FUNCTION_NAME_STRING_CHECKED(FRiveRendererOpenGL,
                                         GetOrCreateRenderContext_Internal),
        [this](FRHICommandListImmediate& RHICmdList) {
            GetOrCreateRenderContext_Internal();
        });
}

void FRiveRendererOpenGL::CreateRenderContext_GameThread()
{
    RIVE_DEBUG_FUNCTION_INDENT;
    check(IsInGameThread());

    GetOrCreateRenderContext_Internal();
}

rive::gpu::RenderContext* FRiveRendererOpenGL::
    GetOrCreateRenderContext_Internal()
{
    RIVE_DEBUG_FUNCTION_INDENT;
    check(IsInGameThread() || IsInRHIThread());
    ENABLE_VERIFY_GL_THREAD;

    if (ensure(IsRHIOpenGL()))
    {
#if WITH_RIVE
        FScopeLock Lock(&ContextsCS);
        if (RenderContext)
        {
            return RenderContext.get();
        }

        DebugLogOpenGLStatus();

        RIVE_DEBUG_VERBOSE("--- OpenGL Console Variables ---");
        RIVE_DEBUG_VERBOSE(" - FOpenGL::SupportsBufferStorage():  %s",
                           FOpenGL::SupportsBufferStorage() ? TEXT("TRUE")
                                                            : TEXT("false"));
        RIVE_DEBUG_VERBOSE(" - FOpenGL::DiscardFrameBufferToResize():  %s",
                           FOpenGL::DiscardFrameBufferToResize()
                               ? TEXT("TRUE")
                               : TEXT("false"));
        if (const IConsoleVariable* CVar =
                IConsoleManager::Get().FindConsoleVariable(
                    TEXT("OpenGL.UsePersistentMappingStagingBuffer")))
        {
            RIVE_DEBUG_VERBOSE(
                " - `OpenGL.UsePersistentMappingStagingBuffer`:  %s",
                CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
        }
        if (const IConsoleVariable* CVar =
                IConsoleManager::Get().FindConsoleVariable(
                    TEXT("OpenGL.UseStagingBuffer")))
        {
            RIVE_DEBUG_VERBOSE(" - `OpenGL.UseStagingBuffer`:  %s",
                               CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
        }
        if (const IConsoleVariable* CVar =
                IConsoleManager::Get().FindConsoleVariable(
                    TEXT("OpenGL.UBODirectWrite")))
        {
            RIVE_DEBUG_VERBOSE(" - `OpenGL.UBODirectWrite`:  %s",
                               CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
        }
        if (const IConsoleVariable* CVar =
                IConsoleManager::Get().FindConsoleVariable(
                    TEXT("OpenGL.UseMapBuffer")))
        {
            RIVE_DEBUG_VERBOSE(" - `OpenGL.UseMapBuffer`:  %s",
                               CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
        }
        if (const IConsoleVariable* CVar =
                IConsoleManager::Get().FindConsoleVariable(
                    TEXT("OpenGL.UseBufferDiscard")))
        {
            RIVE_DEBUG_VERBOSE(" - `OpenGL.UseBufferDiscard`:  %s",
                               CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
        }
        RIVE_DEBUG_VERBOSE(" - `GUseThreadedRendering`:  %s",
                           GUseThreadedRendering ? TEXT("TRUE")
                                                 : TEXT("false"));

        RenderContext = rive::gpu::RenderContextGLImpl::MakeContext();
        RIVE_DEBUG_VERBOSE("RiveRenderContext: %p", RenderContext.get());

        if (!ensure(RenderContext.get() != nullptr))
        {
            UE_LOG(LogRiveRenderer,
                   Error,
                   TEXT("Not able to create an OpenGL Rive Render Context"))
        }
        return RenderContext.get();
#endif // WITH_RIVE
    }
    return nullptr;
}

bool FRiveRendererOpenGL::IsRHIOpenGL()
{
    return GDynamicRHI != nullptr &&
           GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::OpenGL;
}

void FRiveRendererOpenGL::DebugLogOpenGLStatus()
{
    RIVE_DEBUG_FUNCTION_INDENT;

    // -- Macros to make it easier to Log
#define RIVE_DEBUG_GL_BOOL(Param)                                              \
    RIVE_DEBUG_VERBOSE(" - glIsEnabled(%s):   %s",                             \
                       TEXT(#Param),                                           \
                       glIsEnabled(Param) ? TEXT("TRUE") : TEXT("FALSE"));

#define RIVE_DEBUG_GL_INT_2(Param, Val1, Val2)                                 \
    {                                                                          \
        GLint Rive_Debug_GL_Int = 0;                                           \
        glGetIntegerv(Param, &Rive_Debug_GL_Int);                              \
        RIVE_DEBUG_VERBOSE(" - glGetIntegerv(%s):   %s",                       \
                           TEXT(#Param),                                       \
                           Rive_Debug_GL_Int == Val1 ? TEXT(#Val1)             \
                                                     : TEXT(#Val2));           \
    }

#define RIVE_DEBUG_GL_INT_3(Param, Val1, Val2, Val3)                           \
    {                                                                          \
        GLint Rive_Debug_GL_Int = 0;                                           \
        glGetIntegerv(Param, &Rive_Debug_GL_Int);                              \
        RIVE_DEBUG_VERBOSE(                                                    \
            " - glGetIntegerv(%s):   %s",                                      \
            TEXT(#Param),                                                      \
            Rive_Debug_GL_Int == Val1                                          \
                ? TEXT(#Val1)                                                  \
                : (Rive_Debug_GL_Int == Val2 ? TEXT(#Val2) : TEXT(#Val3)));    \
    }

    // -- Actual Logging
    RIVE_DEBUG_GL_BOOL(GL_CULL_FACE)
    RIVE_DEBUG_GL_INT_3(GL_CULL_FACE_MODE,
                        GL_FRONT,
                        GL_BACK,
                        GL_FRONT_AND_BACK);
    RIVE_DEBUG_GL_INT_2(GL_FRONT_FACE, GL_CW, GL_CCW);
    RIVE_DEBUG_GL_BOOL(GL_DEPTH_CLAMP);
    RIVE_DEBUG_GL_BOOL(GL_POLYGON_OFFSET_FILL);
    RIVE_DEBUG_GL_BOOL(GL_POLYGON_OFFSET_LINE);
    RIVE_DEBUG_GL_BOOL(GL_POLYGON_OFFSET_POINT);

    // -- Undef of the Macros
#undef RIVE_DEBUG_GL_BOOL
#undef RIVE_DEBUG_GL_INT_2
#undef RIVE_DEBUG_GL_INT_3
}

#endif // PLATFORM_ANDROID
