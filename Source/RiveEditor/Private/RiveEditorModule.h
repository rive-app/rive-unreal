// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveEditorModule.h"

namespace UE::Rive::Editor::Private
{
    class FRiveEditorModuleModule final : public IRiveEditorModuleModule
    {
        //~ BEGIN : IModuleInterface Interface

    public:

        void StartupModule() override;

        void ShutdownModule() override;

        //~ END : IModuleInterface Interface
    };
}