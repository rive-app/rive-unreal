// Copyright Epic Games, Inc. All Rights Reserved.

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