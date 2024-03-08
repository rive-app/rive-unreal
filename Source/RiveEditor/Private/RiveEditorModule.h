// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IAssetTools.h"
#include "IRiveEditorModule.h"

namespace UE::Rive::Editor::Private
{
    class FRiveEditorModuleModule final : public IRiveEditorModuleModule
    {
        //~ BEGIN : IModuleInterface Interface

    public:
        virtual void StartupModule() override;
        virtual void ShutdownModule() override;
        //~ END : IModuleInterface Interface

    private:
        void RegisterAssetTypeActions(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action);

        EAssetTypeCategories::Type RiveAssetCategory = EAssetTypeCategories::None;
        
        /** All created asset type actions.  Cached here so that we can unregister them during shutdown. */
        TArray< TSharedPtr<IAssetTypeActions> > CreatedAssetTypeActions;
    };
}
