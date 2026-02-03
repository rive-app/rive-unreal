// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRendererModule.h"

class FRiveRenderer;

class FRiveRendererModule : public IRiveRendererModule
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    void StartupRiveRenderer();

private:
    FDelegateHandle OnBeginFrameHandle;
};
