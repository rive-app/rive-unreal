// Copyright Rive, Inc. All rights reserved.

#include "RiveEditorModule.h"

#include "AssetToolsModule.h"
#include "IRiveRendererModule.h"
#include "ISettingsEditorModule.h"
#include "RiveFile_AssetTypeActions.h"
#include "RiveTextureThumbnailRenderer.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Logs/RiveEditorLog.h"
#include "Rive/RiveFile.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#if PLATFORM_WINDOWS
#include "WindowsTargetSettings.h"
#endif

#define LOCTEXT_NAMESPACE "RiveEditorModule"

void UE::Rive::Editor::Private::FRiveEditorModule::StartupModule()
{
	UThumbnailManager::Get().RegisterCustomRenderer(URiveFile::StaticClass(), URiveTextureThumbnailRenderer::StaticClass());

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	
	RiveAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Rive")), LOCTEXT("RiveFileCategory", "Rive"));
	RegisterAssetTypeActions(AssetTools, MakeShareable(new FRiveFile_AssetTypeActions(RiveAssetCategory)));

	OnBeginFrameHandle = FCoreDelegates::OnBeginFrame.AddLambda([this]()
	{
		CheckCurrentRHIAndNotify();
		FCoreDelegates::OnBeginFrame.Remove(OnBeginFrameHandle);
	});
}

void UE::Rive::Editor::Private::FRiveEditorModule::ShutdownModule()
{
	if (RHINotification)
	{
		RHINotification->SetExpireDuration(0.0f);
		RHINotification->ExpireAndFadeout();
		RHINotification.Reset();
	}
	
	if (UObjectInitialized())
	{
		UThumbnailManager::Get().UnregisterCustomRenderer(URiveFile::StaticClass());
	}
	
	// Unregister all the asset types that we registered
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (int32 Index = 0; Index < CreatedAssetTypeActions.Num(); ++Index)
		{
			AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeActions[Index].ToSharedRef());
		}
	}
	CreatedAssetTypeActions.Empty();
}

bool UE::Rive::Editor::Private::FRiveEditorModule::CheckCurrentRHIAndNotify()
{
	Renderer::IRiveRendererModule& Renderer = Renderer::IRiveRendererModule::Get();
	if (Renderer.GetRenderer()) // If we were able to create a Renderer, the RHI is supported
	{
		return true;
	}

#if PLATFORM_WINDOWS
	ERHIInterfaceType ExpectedRHI = ERHIInterfaceType::D3D11;
	EDefaultGraphicsRHI WindowsExpectedRHI = EDefaultGraphicsRHI::DefaultGraphicsRHI_DX11;
	FString ExpectedRHIStr = TEXT("DirectX 11");
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

	const FText NotificationText = ExpectedRHI == ERHIInterfaceType::Null ?
		LOCTEXT("RiveNotAvailable", "Rive is not currently available on this platform") :
		FText::Format(LOCTEXT("RiveWrongRHI_Text", "Rive is not supported for this RHI.\nDo you want to change the RHI of this project to '{0}'"), FText::FromString(ExpectedRHIStr));

	FNotificationInfo Notification(NotificationText);
	Notification.bUseSuccessFailIcons = true;
	Notification.Image = FAppStyle::GetBrush(TEXT("MessageLog.Warning"));
	Notification.bFireAndForget = false;
	Notification.FadeOutDuration = 1.f;

	if (ExpectedRHI != ERHIInterfaceType::Null)
	{
		auto AdjustRHI = [this, 
#if PLATFORM_WINDOWS
						  WindowsExpectedRHI,
#endif
						  ExpectedRHIStr]()
		{
#if PLATFORM_WINDOWS
			UE_LOG(LogRiveEditor, Warning, TEXT("Changing RHI to '%s'"), *ExpectedRHIStr)
			if (const FProperty* DefaultGraphicsRHIProperty = FindFProperty<FProperty>(UWindowsTargetSettings::StaticClass(), GET_MEMBER_NAME_STRING_CHECKED(UWindowsTargetSettings, DefaultGraphicsRHI)))
			{
				UWindowsTargetSettings* WindowsSettings = GetMutableDefault<UWindowsTargetSettings>();
				WindowsSettings->DefaultGraphicsRHI = WindowsExpectedRHI;
				// Then Save the Config
				WindowsSettings->UpdateSinglePropertyInConfigFile(DefaultGraphicsRHIProperty, WindowsSettings->GetDefaultConfigFilename());
				WindowsSettings->TryUpdateDefaultConfigFile();
				ISettingsEditorModule* SettingsEditorModule = FModuleManager::GetModulePtr<ISettingsEditorModule>("SettingsEditor");
				if (SettingsEditorModule)
				{
					SettingsEditorModule->OnApplicationRestartRequired();
				}
			}
			else
			{
				UE_LOG(LogRiveEditor, Error, TEXT("Unable to change the RHI to '%s'"), *ExpectedRHIStr)
			}
#else
			UE_LOG(LogRiveEditor, Error, TEXT("Unable to change RHI for this platform"))
#endif
			if (RHINotification)
			{
				RHINotification->SetExpireDuration(0.0f);
				RHINotification->ExpireAndFadeout();
				RHINotification.Reset();
			}
		};
		
		Notification.ButtonDetails.Add(FNotificationButtonInfo(
			FText::Format(LOCTEXT("RiveWrongRHI_ChangeRHI", "Change RHI to {0}"), FText::FromString(ExpectedRHIStr)),
			FText::Format(LOCTEXT("RiveWrongRHI_ChangeRHI_ToolTip", "Change the RHI of this project to '{0}' for Rive to be available."), FText::FromString(ExpectedRHIStr)),
			FSimpleDelegate::CreateLambda(MoveTemp(AdjustRHI)),
			SNotificationItem::CS_Fail));
	}

	Notification.ButtonDetails.Add(FNotificationButtonInfo(
		LOCTEXT("RiveWrongRHI_Dismiss", "Dismss"), FText::GetEmpty(),
		FSimpleDelegate::CreateLambda([this]()
		{
			if (RHINotification)
			{
				RHINotification->SetExpireDuration(0.0f);
				RHINotification->ExpireAndFadeout();
				RHINotification.Reset();
			}
		}),
		SNotificationItem::CS_Fail));

	RHINotification = FSlateNotificationManager::Get().AddNotification(Notification);
	if (ensure(RHINotification))
	{
		RHINotification->SetCompletionState(SNotificationItem::CS_Fail);
	}
	
	return false;
}

void UE::Rive::Editor::Private::FRiveEditorModule::RegisterAssetTypeActions(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
	AssetTools.RegisterAssetTypeActions(Action);
	CreatedAssetTypeActions.Add(Action);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(UE::Rive::Editor::Private::FRiveEditorModule, RiveEditor)
