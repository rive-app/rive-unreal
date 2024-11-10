// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IAssetTools.h"
#include "IRiveEditorModule.h"

class FRiveEditorModule final : public IRiveEditorModuleModule
{
    //~ BEGIN : IModuleInterface Interface

public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    //~ END : IModuleInterface Interface

    /**
     * Checks the current RHI and create a notification if Rive is not supported
     * for this RHI Returns true if the current RHI is supported.
     */
    bool CheckCurrentRHIAndNotify();

private:
    void RegisterAssetTypeActions(IAssetTools& AssetTools,
                                  TSharedRef<IAssetTypeActions> Action);

    EAssetTypeCategories::Type RiveAssetCategory = EAssetTypeCategories::None;

    /** All created asset type actions.  Cached here so that we can unregister
     * them during shutdown.
     */
    TArray<TSharedPtr<IAssetTypeActions>> CreatedAssetTypeActions;
    TSharedPtr<SNotificationItem> RHINotification;
    FDelegateHandle OnBeginFrameHandle;
};
