// Copyright Rive, Inc. All rights reserved.

#include "RiveStatsModule.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "RiveShaderModule"

void FRiveStatsModule::StartupModule() {}
void FRiveStatsModule::ShutdownModule() {}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRiveStatsModule, RiveStats)
