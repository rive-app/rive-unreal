// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

/*
 * We have a seperate module for loading the shaders, this allows us to move the
 * code that registers the shader directory outside of the rive module which
 * lets us change the load order
 */

class IRiveRenderer;

class FRiveStatsModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
