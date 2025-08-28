// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class IRiveRenderer;

namespace rive
{
class CommandQueue;
}

class IRiveRendererModule : public IModuleInterface
{
    /**
     * Implementation(s)
     */

public:
    /**
     * Singleton-like access to this module's interface.  This is just for
     * convenience! Beware of calling this during the shutdown phase, though.
     * Your module might have been unloaded already.
     *
     * @return Returns singleton instance, loading the module on demand if
     * needed
     */
    static IRiveRendererModule& Get()
    {
        return FModuleManager::LoadModuleChecked<IRiveRendererModule>(
            ModuleName);
    }

    class FRiveRenderer* GetRenderer() const { return RiveRenderer.Get(); }
    static RIVERENDERER_API struct FRiveCommandBuilder& GetCommandBuilder();

    /**
     * Checks to see if this module is loaded and ready.  It is only valid to
     * call Get() during shutdown if IsAvailable() returns true.
     *
     * @return True if the module is loaded and ready to use
     */
    static bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded(ModuleName);
    }

protected:
    TUniquePtr<FRiveRenderer> RiveRenderer;

private:
    static constexpr const TCHAR* ModuleName = TEXT("RiveRenderer");
};
