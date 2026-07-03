// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "RiveRendererModule.h"

#include "RiveRenderer.h"
#include "Logs/RiveRendererLog.h"
#include "RiveRenderer.h"
#include "ShaderCore.h"
#include "RHI.h" // RHIGetRuntimeBindlessConfiguration
#include "Interfaces/IPluginManager.h"
#include "Ore/OrePlatformRHI.h"
#include "rive/command_queue.hpp"

#define LOCTEXT_NAMESPACE "RiveRendererModule"

struct FRiveCommandBuilder& IRiveRendererModule::GetCommandBuilder()
{
    FRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    check(RiveRenderer);
    return RiveRenderer->GetCommandBuilder();
}

void FRiveRendererModule::StartupModule()
{
    RIVE_DEBUG_FUNCTION_INDENT;
    check(GDynamicRHI);

    RiveRenderer = nullptr;
    StartupRiveRenderer();
}

void FRiveRendererModule::ShutdownModule()
{
    if (RiveRenderer)
    {
        RiveRenderer.Reset();
    }
}

void FRiveRendererModule::StartupRiveRenderer()
{
    if (GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Null)
    {
        return;
    }

    // When the running platform binds resources bindlessly (e.g. Metal's Shader
    // Converter), the Ore shaders were cooked with UE's bindless rewrite, so
    // their textures/SRVs/samplers must be bound through the bindless parameter
    // setters at their reflected root-constant offsets rather than by register
    // slot. ERHIBindlessConfiguration::All matches the cook-time condition
    // (ShouldCompileWithBindlessEnabled) for our non-ray-tracing global
    // shaders. See OrePlatformRHI.h and RiveOreShaderHandler's register-strip.
    if (RHIGetRuntimeBindlessConfiguration(GMaxRHIShaderPlatform) ==
        ERHIBindlessConfiguration::All)
    {
        GRHIOreNeedsReflectionSlotRemap = true;
        GRHIOreNeedsBindlessParameters = true;
    }

    UE_LOG(LogRiveRenderer, Display, TEXT("Rive running on RHI."));
    RiveRenderer = MakeUnique<FRiveRenderer>();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRiveRendererModule, RiveRenderer)
