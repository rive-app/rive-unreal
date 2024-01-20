// Copyright Epic Games, Inc. All Rights Reserved.

#include "RiveRendererModule.h"
#include "RiveRenderer.h"

#include "Framework/Application/SlateApplication.h"
#if !PLATFORM_ANDROID
// #include "Platform/RiveRendererD3D11.h"
#else

#endif
#define LOCTEXT_NAMESPACE "RiveRendererModule"

void UE::Rive::Renderer::Private::FRiveRendererModule::StartupModule()
{
    // Create Platform Specific Renderer
    RiveRenderer = nullptr;
    switch (RHIGetInterfaceType())
    {
    case ERHIInterfaceType::D3D11:
        {
#if !PLATFORM_ANDROID
            // RiveRenderer = MakeShared<FRiveRendererD3D11>();
#endif
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
    case ERHIInterfaceType::OpenGL:
        {
            UE_LOG(LogTemp, Error, TEXT("---OPEN GL---"))
            break;
        }
    default:
        break;
    }


    FCoreDelegates::OnFEngineLoopInitComplete.AddLambda([this]()
    {
        if (RiveRenderer.IsValid())
        {
            RiveRenderer->Initialize();
        }
    });
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
