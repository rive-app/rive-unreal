// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class FRiveEditorNodesModule final : public IModuleInterface
{
    //~ BEGIN : IModuleInterface Interface

public:
    virtual void StartupModule() override;

    virtual void ShutdownModule() override;

    //~ END : IModuleInterface Interface

    /**
     * Implementation(s)
     */

private:
    void TestRiveIntegration();
};
