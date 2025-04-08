// Copyright Rive, Inc. All rights reserved.

#include "RiveEditorModule.h"

#include "AssetToolsModule.h"
#include "IRiveRendererModule.h"
#include "ISettingsEditorModule.h"
#include "ISettingsModule.h"
#include "RiveFileDetailCustomization.h"
#include "RiveFileAssetTypeActions.h"
#include "RiveFileThumbnailRenderer.h"
#include "RiveRendererSettings.h"
#include "RiveTextureObjectAssetTypeActions.h"
#include "RiveTextureObjectThumbnailRenderer.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Logs/RiveEditorLog.h"
#include "Rive/RiveFile.h"
#include "Rive/RiveTextureObject.h"
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
    UThumbnailManager::Get().RegisterCustomRenderer(
        URiveTextureObject::StaticClass(),
        URiveTextureObjectThumbnailRenderer::StaticClass());

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
    RegisterAssetTypeActions(
        AssetTools,
        MakeShareable(
            new FRiveTextureObjectAssetTypeActions(RiveAssetCategory)));

    OnBeginFrameHandle = FCoreDelegates::OnBeginFrame.AddLambda([this]() {
        CheckCurrentRHIAndNotify();
        FCoreDelegates::OnBeginFrame.Remove(OnBeginFrameHandle);
    });

    // Register settings
    if (ISettingsModule* SettingsModule =
            FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
    {
        SettingsModule->RegisterSettings(
            "Project",
            "Plugins",
            "Rive",
            LOCTEXT("RiveRendererSettingsName", "Rive"),
            LOCTEXT("RiveRendererDescription", "Configure Rive settings"),
            GetMutableDefault<URiveRendererSettings>());
    }

    GetMutableDefault<URiveRendererSettings>()->OnSettingChanged().AddLambda(
        [this](UObject*, struct FPropertyChangedEvent&) {
            FUnrealEdMisc::Get().RestartEditor();
        });
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
        UThumbnailManager::Get().UnregisterCustomRenderer(
            URiveTextureObject::StaticClass());
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

bool FRiveEditorModule::CheckCurrentRHIAndNotify()
{
    IRiveRendererModule& Renderer = IRiveRendererModule::Get();
    if (Renderer.GetRenderer()) // If we were able to create a Renderer, the RHI
                                // is supported
    {
        return true;
    }
    else
    {
        UE_LOG(LogRiveEditor,
               Error,
               TEXT("CheckCurrentRHIAndNotify: No renderer found."))
    }

#if PLATFORM_WINDOWS
    return true; // DX11, DX12, and Vulkan are all supported.
#elif PLATFORM_APPLE
    ERHIInterfaceType ExpectedRHI = ERHIInterfaceType::Metal;
    FString ExpectedRHIStr = TEXT("Metal");
#elif PLATFORM_ANDROID
    ERHIInterfaceType ExpectedRHI = ERHIInterfaceType::OpenGL;
    FString ExpectedRHIStr = TEXT("OpenGL");
#else
    ERHIInterfaceType ExpectedRHI = ERHIInterfaceType::Null;
    FString ExpectedRHIStr = "";
#endif

#if !PLATFORM_WINDOWS
    const FText NotificationText =
        ExpectedRHI == ERHIInterfaceType::Null
            ? LOCTEXT("RiveNotAvailable",
                      "Rive is not currently available on this platform")
            : FText::Format(LOCTEXT("RiveWrongRHI_Text",
                                    "Rive is not supported for this RHI.\nDo "
                                    "you want to change "
                                    "the RHI of this project to '{0}'"),
                            FText::FromString(ExpectedRHIStr));

    FNotificationInfo Notification(NotificationText);
    Notification.bUseSuccessFailIcons = true;
    Notification.Image = FAppStyle::GetBrush(TEXT("MessageLog.Warning"));
    Notification.bFireAndForget = false;
    Notification.FadeOutDuration = 1.f;

    if (ExpectedRHI != ERHIInterfaceType::Null)
    {
        auto AdjustRHI = [this, ExpectedRHIStr]() {
            UE_LOG(LogRiveEditor,
                   Error,
                   TEXT("Unable to change RHI for this platform"))
            if (RHINotification)
            {
                RHINotification->SetExpireDuration(0.0f);
                RHINotification->ExpireAndFadeout();
                RHINotification.Reset();
            }
        };

        Notification.ButtonDetails.Add(FNotificationButtonInfo(
            FText::Format(
                LOCTEXT("RiveWrongRHI_ChangeRHI", "Change RHI to {0}"),
                FText::FromString(ExpectedRHIStr)),
            FText::Format(LOCTEXT("RiveWrongRHI_ChangeRHI_ToolTip",
                                  "Change the RHI of this project to '{0}' for "
                                  "Rive to be available."),
                          FText::FromString(ExpectedRHIStr)),
            FSimpleDelegate::CreateLambda(MoveTemp(AdjustRHI)),
            SNotificationItem::CS_Fail));
    }

    Notification.ButtonDetails.Add(FNotificationButtonInfo(
        LOCTEXT("RiveWrongRHI_Dismiss", "Dismss"),
        FText::GetEmpty(),
        FSimpleDelegate::CreateLambda([this]() {
            if (RHINotification)
            {
                RHINotification->SetExpireDuration(0.0f);
                RHINotification->ExpireAndFadeout();
                RHINotification.Reset();
            }
        }),
        SNotificationItem::CS_Fail));

    RHINotification =
        FSlateNotificationManager::Get().AddNotification(Notification);
    if (ensure(RHINotification))
    {
        RHINotification->SetCompletionState(SNotificationItem::CS_Fail);
    }

    return false;
#endif
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
