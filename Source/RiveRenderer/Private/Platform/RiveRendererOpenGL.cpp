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
#include "RiveCore/Public/PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#include "rive/pls/gl/pls_render_context_gl_impl.hpp"
#include "rive/pls/pls_renderer.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

// ---------------------------------------------------
// --------------- FRiveRendererOpenGL ---------------
// ---------------------------------------------------

void UE::Rive::Renderer::Private::FRiveRendererOpenGL::Initialize()
{
	check(IsInGameThread());
	RIVE_DEBUG_FUNCTION_INDENT
	
	{
		FString glVersionStr = ANSI_TO_TCHAR((const ANSICHAR*) glGetString(GL_VERSION));
		RIVE_DEBUG_VERBOSE("GAMETHREAD: glVersionStr %s", *glVersionStr);
            
		ENQUEUE_RENDER_COMMAND(URiveFileInitialize_RenderThread)(
		[](FRHICommandListImmediate& RHICmdList)
		{
			FString glVersionStr = ANSI_TO_TCHAR((const ANSICHAR*) glGetString(GL_VERSION));
			RIVE_DEBUG_VERBOSE("RENDERTHREAD: glVersionStr %s", *glVersionStr);
			RHICmdList.EnqueueLambda([](FRHICommandListImmediate&)
			{
				FString glVersionStr = ANSI_TO_TCHAR((const ANSICHAR*) glGetString(GL_VERSION));
				RIVE_DEBUG_VERBOSE("RHITHREAD: glVersionStr %s", *glVersionStr);
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
		
			CreatePLSContext_GameThread();
			InitializationState = ERiveInitState::Initialized;
		}
		
		OnInitializedDelegate.Broadcast(this);
	}
	else
	{
		FRiveRenderer::Initialize();
	}
}

rive::pls::PLSRenderContext* UE::Rive::Renderer::Private::FRiveRendererOpenGL::GetPLSRenderContextPtr()
{
	RIVE_DEBUG_FUNCTION_INDENT;
	if (ensure(IsRHIOpenGL()))
	{
		FScopeLock Lock(&ContextsCS);
		if (PLSRenderContext)
		{
			return PLSRenderContext.get();
		}
	}
	
	UE_LOG(LogRiveRenderer, Error, TEXT("Rive PLS Render Context is uninitialized for the Current OpenGL Context."));
	return nullptr;
}

TSharedPtr<UE::Rive::Renderer::IRiveRenderTarget> UE::Rive::Renderer::Private::FRiveRendererOpenGL::CreateTextureTarget_GameThread(const FName& InRiveName, UTexture2DDynamic* InRenderTarget)
{
	RIVE_DEBUG_FUNCTION_INDENT;
	check(IsInGameThread());
	
	const TSharedPtr<FRiveRenderTargetOpenGL> RiveRenderTarget = MakeShared<FRiveRenderTargetOpenGL>(SharedThis(this), InRiveName, InRenderTarget);
	RenderTargets.Add(InRiveName, RiveRenderTarget);
	
	return RiveRenderTarget;
}

DECLARE_GPU_STAT_NAMED(CreatePLSContext, TEXT("CreatePLSContext_RenderThread"));
void UE::Rive::Renderer::Private::FRiveRendererOpenGL::CreatePLSContext_RenderThread(FRHICommandListImmediate& RHICmdList)
{
	RIVE_DEBUG_FUNCTION_INDENT;
	check(IsInRenderingThread());

	RHICmdList.EnqueueLambda(
		GET_FUNCTION_NAME_STRING_CHECKED(FRiveRendererOpenGL, GetOrCreatePLSRenderContextPtr_Internal),
		[this](FRHICommandListImmediate& RHICmdList)
		{
			GetOrCreatePLSRenderContextPtr_Internal();
		});
}

void UE::Rive::Renderer::Private::FRiveRendererOpenGL::CreatePLSContext_GameThread()
{
	RIVE_DEBUG_FUNCTION_INDENT;
	check(IsInGameThread());
	
	GetOrCreatePLSRenderContextPtr_Internal();
}


rive::pls::PLSRenderContext* UE::Rive::Renderer::Private::FRiveRendererOpenGL::GetOrCreatePLSRenderContextPtr_Internal()
{
	RIVE_DEBUG_FUNCTION_INDENT;
	check(IsInGameThread() || IsInRHIThread());
	ENABLE_VERIFY_GL_THREAD;
	
	if (ensure(IsRHIOpenGL()))
	{
#if WITH_RIVE
		FScopeLock Lock(&ContextsCS);
		if (PLSRenderContext)
		{
			return PLSRenderContext.get();
		}
		
		DebugLogOpenGLStatus();
		
		RIVE_DEBUG_VERBOSE("--- OpenGL Console Variables ---");
		RIVE_DEBUG_VERBOSE(" - FOpenGL::SupportsBufferStorage():  %s", FOpenGL::SupportsBufferStorage() ? TEXT("TRUE") : TEXT("false"));
		RIVE_DEBUG_VERBOSE(" - FOpenGL::DiscardFrameBufferToResize():  %s", FOpenGL::DiscardFrameBufferToResize() ? TEXT("TRUE") : TEXT("false"));
		if (const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("OpenGL.UsePersistentMappingStagingBuffer")))
		{
			RIVE_DEBUG_VERBOSE(" - `OpenGL.UsePersistentMappingStagingBuffer`:  %s", CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
		}
		if (const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("OpenGL.UseStagingBuffer")))
		{
			RIVE_DEBUG_VERBOSE(" - `OpenGL.UseStagingBuffer`:  %s", CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
		}
		if (const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("OpenGL.UBODirectWrite")))
		{
			RIVE_DEBUG_VERBOSE(" - `OpenGL.UBODirectWrite`:  %s", CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
		}
		if (const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("OpenGL.UseMapBuffer")))
		{
			RIVE_DEBUG_VERBOSE(" - `OpenGL.UseMapBuffer`:  %s", CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
		}
		if (const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("OpenGL.UseBufferDiscard")))
		{
			RIVE_DEBUG_VERBOSE(" - `OpenGL.UseBufferDiscard`:  %s", CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
		}
		RIVE_DEBUG_VERBOSE(" - `GUseThreadedRendering`:  %s", GUseThreadedRendering ? TEXT("TRUE") : TEXT("false"));
		
		
		PLSRenderContext = rive::pls::PLSRenderContextGLImpl::MakeContext();
		RIVE_DEBUG_VERBOSE("PLSRenderContext: %p", PLSRenderContext.get());
		
		if (!ensure(PLSRenderContext.get() != nullptr))
		{
			UE_LOG(LogRiveRenderer, Error, TEXT("Not able to create an OpenGL PLS Render Context"))
		}
		return PLSRenderContext.get();
#endif // WITH_RIVE
	}
	return nullptr;
}

bool UE::Rive::Renderer::Private::FRiveRendererOpenGL::IsRHIOpenGL()
{
	return GDynamicRHI != nullptr && GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::OpenGL;
}


void UE::Rive::Renderer::Private::FRiveRendererOpenGL::DebugLogOpenGLStatus()
{
	RIVE_DEBUG_FUNCTION_INDENT;
	
	// -- Macros to make it easier to Log
#define RIVE_DEBUG_GL_BOOL(Param) RIVE_DEBUG_VERBOSE(" - glIsEnabled(%s):   %s", TEXT(#Param),  glIsEnabled(Param) ? TEXT("TRUE") : TEXT("FALSE"));
	
#define RIVE_DEBUG_GL_INT_2(Param, Val1, Val2) \
	{ \
	GLint Rive_Debug_GL_Int = 0; \
	glGetIntegerv(Param, &Rive_Debug_GL_Int); \
	RIVE_DEBUG_VERBOSE(" - glGetIntegerv(%s):   %s", TEXT(#Param),  Rive_Debug_GL_Int == Val1 ? TEXT(#Val1) : TEXT(#Val2)); \
	}
	
#define RIVE_DEBUG_GL_INT_3(Param, Val1, Val2, Val3) \
	{ \
	GLint Rive_Debug_GL_Int = 0; \
	glGetIntegerv(Param, &Rive_Debug_GL_Int); \
	RIVE_DEBUG_VERBOSE(" - glGetIntegerv(%s):   %s", TEXT(#Param),  Rive_Debug_GL_Int == Val1 ? TEXT(#Val1) : (Rive_Debug_GL_Int == Val2 ? TEXT(#Val2) : TEXT(#Val3))); \
	}
	
	// -- Actual Logging
	RIVE_DEBUG_GL_BOOL(GL_CULL_FACE)
	RIVE_DEBUG_GL_INT_3(GL_CULL_FACE_MODE, GL_FRONT, GL_BACK, GL_FRONT_AND_BACK);
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
