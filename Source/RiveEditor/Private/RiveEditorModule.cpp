// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "RiveEditorModule.h"

#include "AssetToolsModule.h"
#include "IRiveRendererModule.h"
#include "ISettingsEditorModule.h"
#include "ISettingsModule.h"
#include "PropertyEditorModule.h"
#include "RiveFileDetailCustomization.h"
#include "RiveFileAssetTypeActions.h"
#include "RiveFileThumbnailRenderer.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Logs/RiveEditorLog.h"
#include "Rive/RiveFile.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#if PLATFORM_WINDOWS
#include "WindowsTargetSettings.h"
#endif

#define LOCTEXT_NAMESPACE "RiveEditorModule"

void FRiveEditorModule::StartupModule()
{
    UThumbnailManager::Get().RegisterCustomRenderer(
        URiveFile::StaticClass(),
        URiveFileThumbnailRenderer::StaticClass());

    FPropertyEditorModule& PropertyModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>(
            "PropertyEditor");
    PropertyModule.RegisterCustomClassLayout(
        URiveFile::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(
            &FRiveFileDetailCustomization::MakeInstance));

    IAssetTools& AssetTools =
        FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools")
            .Get();
    RiveAssetCategory = AssetTools.RegisterAdvancedAssetCategory(
        FName(TEXT("Rive")),
        LOCTEXT("RiveFileCategory", "Rive"));
    RegisterAssetTypeActions(
        AssetTools,
        MakeShareable(new FRiveFileAssetTypeActions(RiveAssetCategory)));
}

void FRiveEditorModule::ShutdownModule()
{
    if (RHINotification)
    {
        RHINotification->SetExpireDuration(0.0f);
        RHINotification->ExpireAndFadeout();
        RHINotification.Reset();
    }

    if (UObjectInitialized())
    {
        UThumbnailManager::Get().UnregisterCustomRenderer(
            URiveFile::StaticClass());
    }

    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        FPropertyEditorModule& PropertyModule =
            FModuleManager::LoadModuleChecked<FPropertyEditorModule>(
                "PropertyEditor");
        PropertyModule.UnregisterCustomClassLayout(
            URiveFile::StaticClass()->GetFName());
    }

    // Unregister all the asset types that we registered
    if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
    {
        IAssetTools& AssetTools =
            FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools")
                .Get();
        for (int32 Index = 0; Index < CreatedAssetTypeActions.Num(); ++Index)
        {
            AssetTools.UnregisterAssetTypeActions(
                CreatedAssetTypeActions[Index].ToSharedRef());
        }
    }
    CreatedAssetTypeActions.Empty();

    if (ISettingsModule* SettingsModule =
            FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
    {
        SettingsModule->UnregisterSettings("Project", "Plugins", "Rive");
    }
}

void FRiveEditorModule::RegisterAssetTypeActions(
    IAssetTools& AssetTools,
    TSharedRef<IAssetTypeActions> Action)
{
    AssetTools.RegisterAssetTypeActions(Action);
    CreatedAssetTypeActions.Add(Action);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRiveEditorModule, RiveEditor)
