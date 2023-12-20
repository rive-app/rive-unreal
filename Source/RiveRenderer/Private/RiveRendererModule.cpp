// Copyright Epic Games, Inc. All Rights Reserved.

#include "RiveRendererModule.h"
#include "RiveRenderer.h"

#define LOCTEXT_NAMESPACE "RiveRendererModule"

void UE::Rive::Renderer::Private::FRiveRendererModule::StartupModule()
{
    RiveRenderer = MakeUnique<FRiveRenderer>();
}

void UE::Rive::Renderer::Private::FRiveRendererModule::ShutdownModule()
{
}

UE::Rive::Renderer::IRiveRenderer* UE::Rive::Renderer::Private::FRiveRendererModule::GetRenderer()
{
    return RiveRenderer.Get();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(UE::Rive::Renderer::Private::FRiveRendererModule, RiveRenderer)
