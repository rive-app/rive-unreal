// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class IRiveModule : public IModuleInterface
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
    static inline IRiveModule& Get()
    {
        return FModuleManager::LoadModuleChecked<IRiveModule>(ModuleName);
    }

    /**
     * Checks to see if this module is loaded and ready.  It is only valid to
     * call Get() during shutdown if IsAvailable() returns true.
     *
     * @return True if the module is loaded and ready to use
     */
    static inline bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded(ModuleName);
    }

    /**
     * Attribute(s)
     */

private:
    static constexpr const TCHAR* ModuleName = TEXT("Rive");
};
