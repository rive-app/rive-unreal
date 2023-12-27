// Copyright Epic Games, Inc. All Rights Reserved.

#include "RiveRendererModule.h"
#include "RiveRenderer.h"

#include "Framework/Application/SlateApplication.h"
#include "Platform/RiveRendererD3D11.h"
#define LOCTEXT_NAMESPACE "RiveRendererModule"

void UE::Rive::Renderer::Private::FRiveRendererModule::StartupModule()
{
    // Create Platform Specific Renderer
    RiveRenderer = nullptr;
    switch (RHIGetInterfaceType())
    {
    case ERHIInterfaceType::D3D11:
        {
            RiveRenderer = MakeUnique<FRiveRendererD3D11>();
            break;
        }
    case ERHIInterfaceType::D3D12:
        {
            break;
        }
    case ERHIInterfaceType::Vulkan:
        {
            break;
        }
    default:
        break;
    }
    
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
