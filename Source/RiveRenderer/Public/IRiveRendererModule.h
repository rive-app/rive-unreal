// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class IRiveRenderer;

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
    static inline IRiveRendererModule& Get()
    {
        return FModuleManager::LoadModuleChecked<IRiveRendererModule>(
            ModuleName);
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

    virtual class IRiveRenderer* GetRenderer() = 0;

    virtual void CallOrRegister_OnRendererInitialized(
        FSimpleMulticastDelegate::FDelegate&& Delegate) = 0;

    /**
     * Attribute(s)
     */
    // TODO. REMOVE IT!! Temporary switches for Android testng
    static bool RunInGameThread()
    {
#if PLATFORM_ANDROID
        return true;
#endif // PLATFORM_ANDROID
        return false;
    }

private:
    static constexpr const TCHAR* ModuleName = TEXT("RiveRenderer");
};
