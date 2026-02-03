// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "RiveModule.h"

#include "Interfaces/IPluginManager.h"
#include "Logs/RiveLog.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "Misc/CoreDelegates.h"
#include "Misc/MessageDialog.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Internationalization/Internationalization.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/runtime_header.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

#define LOCTEXT_NAMESPACE "FRiveModule"

void FRiveModule::StartupModule() {}

void FRiveModule::ShutdownModule() { ResetAllShaderSourceDirectoryMappings(); }

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRiveModule, Rive)
