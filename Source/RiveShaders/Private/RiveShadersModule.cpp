// Copyright Rive, Inc. All rights reserved.

#include "RiveShadersModule.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "RiveShaderModule"

void FRiveShadersModule::StartupModule()
{
    FString PluginShaderDir = FPaths::Combine(
        IPluginManager::Get().FindPlugin(TEXT("Rive"))->GetBaseDir(),
        TEXT("Shaders"));
    AddShaderSourceDirectoryMapping(TEXT("/Plugin/Rive"), PluginShaderDir);
}

void FRiveShadersModule::ShutdownModule() {}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRiveShadersModule, RiveShaders)
