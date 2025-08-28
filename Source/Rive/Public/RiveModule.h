// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#pragma once

#include "IRiveModule.h"

class FRiveModule final : public IRiveModule
{
    //~ BEGIN : IModuleInterface Interface

public:
    virtual void StartupModule() override;

    virtual void ShutdownModule() override;

    //~ END : IModuleInterface Interface

    /**
     * Implementation(s)
     */
};
