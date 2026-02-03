// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "RiveRendererModule.h"

#include "RiveRenderer.h"
#include "Logs/RiveRendererLog.h"
#include "Platform/RiveRendererRHI.h"
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

    UE_LOG(LogRiveRenderer, Display, TEXT("Rive running on RHI."))
    RiveRenderer = MakeUnique<FRiveRendererRHI>();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRiveRendererModule, RiveRenderer)
