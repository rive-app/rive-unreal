// Copyright Rive, Inc. All rights reserved.

#include "RiveWidget/RiveFullScreenUserWidget.h"

#include "Blueprint/UserWidget.h"
#include "Engine/UserInterfaceSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Logs/RiveLog.h"
#include "Slate/SceneViewport.h"
#include "UMG/RiveWidget.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SDPIScaler.h"

#if WITH_EDITOR
#include "LevelEditor.h"
#include "LevelEditorViewport.h"
#include "SLevelViewport.h"
#endif // WITH_EDITOR

/////////////////////////////////////////////////////
// Internal helper
namespace
{
	const FName NAME_LevelEditorName = "LevelEditor";

	namespace RiveFullScreenUserWidgetPrivate
	{
		/**
		 * Class made to handle world cleanup and hide/cleanup active UserWidget to avoid touching public headers
		 */
		class FWorldCleanupListener
		{
		public:

			static FWorldCleanupListener* Get()
			{
				static FWorldCleanupListener Instance;
				return &Instance;
			}

			/** Disallow Copying / Moving */
			UE_NONCOPYABLE(FWorldCleanupListener);

			~FWorldCleanupListener()
			{
				FWorldDelegates::OnWorldCleanup.RemoveAll(this);
			}

			void AddWidget(URiveFullScreenUserWidget* InWidget)
			{
				WidgetsToHide.AddUnique(InWidget);
			}

			void RemoveWidget(URiveFullScreenUserWidget* InWidget)
			{
				WidgetsToHide.RemoveSingleSwap(InWidget, false);
			}

		private:

			FWorldCleanupListener()
			{
				FWorldDelegates::OnWorldCleanup.AddRaw(this, &FWorldCleanupListener::OnWorldCleanup);
			}

			void OnWorldCleanup(UWorld* InWorld, bool bSessionEnded, bool bCleanupResources)
			{
				for (auto WeakWidgetIter = WidgetsToHide.CreateIterator(); WeakWidgetIter; ++WeakWidgetIter)
				{
					TWeakObjectPtr<URiveFullScreenUserWidget>& WeakWidget = *WeakWidgetIter;
					
					if (URiveFullScreenUserWidget* Widget = WeakWidget.Get())
					{
						if (Widget->IsDisplayed()
							&& Widget->GetWidget()
							&& (Widget->GetWidget()->GetWorld() == InWorld))
						{
							// Remove first since Hide removes object from the list
							WeakWidgetIter.RemoveCurrent();
					
							Widget->Hide();
						}
					}
					else
					{
						WeakWidgetIter.RemoveCurrent();
					}
				}
			}

		private:

			TArray<TWeakObjectPtr<URiveFullScreenUserWidget>> WidgetsToHide;
		};
	}
}

/////////////////////////////////////////////////////
// FRiveFullScreenUserWidget_Viewport

bool FRiveFullScreenUserWidget_Viewport::Display(UWorld* World, UUserWidget* InWidget, TAttribute<float> InDPIScale)
{
	const TSharedPtr<SConstraintCanvas> FullScreenWidgetPinned = FullScreenCanvasWidget.Pin();
	
	if (InWidget == nullptr || World == nullptr || FullScreenWidgetPinned.IsValid())
	{
		return false;
	}
	
	const TSharedRef<SConstraintCanvas> FullScreenCanvas = SNew(SConstraintCanvas);
	
	FullScreenCanvas->AddSlot()
		.Offset(FMargin(0, 0, 0, 0))
		.Anchors(FAnchors(0, 0, 1, 1))
		.Alignment(FVector2D(0, 0))
		[
			SNew(SDPIScaler)
			.DPIScale(MoveTemp(InDPIScale))
			[
				InWidget->TakeWidget()
			]
		];

	
	UGameViewportClient* ViewportClient = World->GetGameViewport();
	
	const bool bCanUseGameViewport = ViewportClient && World->IsGameWorld();
	
	if (bCanUseGameViewport)
	{
		FullScreenCanvasWidget = FullScreenCanvas;
	
		ViewportClient->AddViewportWidgetContent(FullScreenCanvas);
		
		return true;
	}

#if WITH_EDITOR

	const TSharedPtr<FSceneViewport> PinnedTargetViewport = EditorTargetViewport.Pin();
	
	for (FLevelEditorViewportClient* Client : GEditor->GetLevelViewportClients())
	{
		const TSharedPtr<SLevelViewport> LevelViewport = StaticCastSharedPtr<SLevelViewport>(Client->GetEditorViewportWidget());
	
		if (LevelViewport.IsValid() && LevelViewport->GetSceneViewport() == PinnedTargetViewport)
		{
			LevelViewport->AddOverlayWidget(FullScreenCanvas);
			
			FullScreenCanvasWidget = FullScreenCanvas;
		
			OverlayWidgetLevelViewport = LevelViewport;
			
			return true;
		}
	}

#endif // WITH_EDITOR

	return false;
}

void FRiveFullScreenUserWidget_Viewport::Hide(UWorld* World)
{
	TSharedPtr<SConstraintCanvas> FullScreenWidgetPinned = FullScreenCanvasWidget.Pin();
	
	if (FullScreenWidgetPinned.IsValid())
	{
		// Remove from Viewport and Fullscreen, in case the settings changed before we had the chance to hide.
		UGameViewportClient* ViewportClient = World ? World->GetGameViewport() : nullptr;
	
		if (ViewportClient)
		{
			ViewportClient->RemoveViewportWidgetContent(FullScreenWidgetPinned.ToSharedRef());
		}

#if WITH_EDITOR
		
		if (const TSharedPtr<SLevelViewport> OverlayWidgetLevelViewportPinned = OverlayWidgetLevelViewport.Pin())
		{
			OverlayWidgetLevelViewportPinned->RemoveOverlayWidget(FullScreenWidgetPinned.ToSharedRef());
		}
		
		OverlayWidgetLevelViewport.Reset();

#endif // WITH_EDITOR

		FullScreenCanvasWidget.Reset();
	}
}


URiveFullScreenUserWidget::URiveFullScreenUserWidget()
	: CurrentDisplayType(ERiveWidgetDisplayType::Inactive)
	, bDisplayRequested(false)
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> M_PostProcessMaterial(TEXT("/Rive/Materials/WidgetPostProcessMaterial"));

	if (M_PostProcessMaterial.Succeeded())
	{
		PostProcessDisplayTypeWithBlendMaterial.PostProcessMaterial = M_PostProcessMaterial.Object;
	}
}

void URiveFullScreenUserWidget::BeginDestroy()
{
	Hide();

	Super::BeginDestroy();
}

#if WITH_EDITOR

void URiveFullScreenUserWidget::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	FProperty* Property = PropertyChangedEvent.MemberProperty;

	if (Property && PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		static FName NAME_WidgetClass = GET_MEMBER_NAME_CHECKED(URiveFullScreenUserWidget, WidgetClass);
		
		static FName NAME_EditorDisplayType = GET_MEMBER_NAME_CHECKED(URiveFullScreenUserWidget, EditorDisplayType);
		
		static FName NAME_PostProcessMaterial = GET_MEMBER_NAME_CHECKED(FRiveFullScreenUserWidget_PostProcess, PostProcessMaterial);
		
		static FName NAME_WidgetDrawSize = GET_MEMBER_NAME_CHECKED(FRiveFullScreenUserWidget_PostProcess, WidgetDrawSize);
		
		static FName NAME_WindowFocusable = GET_MEMBER_NAME_CHECKED(FRiveFullScreenUserWidget_PostProcess, bWindowFocusable);
		
		static FName NAME_WindowVisibility = GET_MEMBER_NAME_CHECKED(FRiveFullScreenUserWidget_PostProcess, WindowVisibility);
		
		static FName NAME_ReceiveHardwareInput = GET_MEMBER_NAME_CHECKED(FRiveFullScreenUserWidget_PostProcess, bReceiveHardwareInput);
		
		static FName NAME_RenderTargetBackgroundColor = GET_MEMBER_NAME_CHECKED(FRiveFullScreenUserWidget_PostProcess, RenderTargetBackgroundColor);
		
		static FName NAME_RenderTargetBlendMode = GET_MEMBER_NAME_CHECKED(FRiveFullScreenUserWidget_PostProcess, RenderTargetBlendMode);
		
		static FName NAME_PostProcessTintColorAndOpacity = GET_MEMBER_NAME_CHECKED(FRiveFullScreenUserWidget_PostProcess, PostProcessTintColorAndOpacity);
		
		static FName NAME_PostProcessOpacityFromTexture = GET_MEMBER_NAME_CHECKED(FRiveFullScreenUserWidget_PostProcess, PostProcessOpacityFromTexture);

		if (Property->GetFName() == NAME_WidgetClass
			|| Property->GetFName() == NAME_EditorDisplayType
			|| Property->GetFName() == NAME_PostProcessMaterial
			|| Property->GetFName() == NAME_WidgetDrawSize
			|| Property->GetFName() == NAME_WindowFocusable
			|| Property->GetFName() == NAME_WindowVisibility
			|| Property->GetFName() == NAME_ReceiveHardwareInput
			|| Property->GetFName() == NAME_RenderTargetBackgroundColor
			|| Property->GetFName() == NAME_RenderTargetBlendMode
			|| Property->GetFName() == NAME_PostProcessTintColorAndOpacity
			|| Property->GetFName() == NAME_PostProcessOpacityFromTexture
			)
		{
			const bool bWasRequestedDisplay = bDisplayRequested;
		
			UWorld* CurrentWorld = World.Get();
			
			Hide();
			
			if (bWasRequestedDisplay && CurrentWorld)
			{
				Display(CurrentWorld);
			}
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif // WITH_EDITOR

bool URiveFullScreenUserWidget::Display(UWorld* InWorld)
{
	bDisplayRequested = true;

	World = InWorld;

#if WITH_EDITOR

	if (!EditorTargetViewport.IsValid() && !World->IsGameWorld())
	{
		UE_LOG(LogRive, Log, TEXT("No TargetViewport set. Defaulting to FLevelEditorModule::GetFirstActiveLevelViewport."));
		
		if (FModuleManager::Get().IsModuleLoaded(NAME_LevelEditorName))
		{
			FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(NAME_LevelEditorName);
		
			const TSharedPtr<SLevelViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveLevelViewport();
			
			EditorTargetViewport = ActiveLevelViewport ? ActiveLevelViewport->GetSharedActiveViewport() : nullptr;
		}

		if (!EditorTargetViewport.IsValid())
		{
			UE_LOG(LogRive, Error, TEXT("FLevelEditorModule::GetFirstActiveLevelViewport found no level viewport. URiveFullScreenUserWidget will not display."));
		
			return false;
		}
	}
	
	// Make sure that each display type has also received the EditorTargetViewport
	SetEditorTargetViewport(EditorTargetViewport);

#endif // WITH_EDITOR

	bool bWasAdded = false;

	if (WidgetClass && InWorld && ShouldDisplay(InWorld) && CurrentDisplayType == ERiveWidgetDisplayType::Inactive)
	{
		const bool bCreatedWidget = InitWidget();
		
		if (!bCreatedWidget)
		{
			UE_LOG(LogRive, Error, TEXT("Failed to create subwidget for URiveFullScreenUserWidget."));
		
			return false;
		}
		
		CurrentDisplayType = GetDisplayType(InWorld);

		TAttribute<float> GetDpiScaleAttribute = TAttribute<float>::CreateLambda([WeakThis = TWeakObjectPtr<URiveFullScreenUserWidget>(this)]()
		{
			return WeakThis.IsValid() ? WeakThis->GetViewportDPIScale() : 1.f;
		});

		if (CurrentDisplayType == ERiveWidgetDisplayType::Viewport)
		{
			bWasAdded = ViewportDisplayType.Display(InWorld, Widget, MoveTemp(GetDpiScaleAttribute));
		}
		else if (CurrentDisplayType == ERiveWidgetDisplayType::PostProcessWithBlendMaterial)
		{
			bWasAdded = PostProcessDisplayTypeWithBlendMaterial.Display(InWorld, Widget, false, MoveTemp(GetDpiScaleAttribute));
		}
		else if (CurrentDisplayType == ERiveWidgetDisplayType::PostProcessSceneViewExtension)
		{
			bWasAdded = PostProcessWithSceneViewExtensions.Display(InWorld, Widget, MoveTemp(GetDpiScaleAttribute));
		}

		if (bWasAdded)
		{
			FWorldDelegates::LevelRemovedFromWorld.AddUObject(this, &URiveFullScreenUserWidget::OnLevelRemovedFromWorld);
		
			FWorldDelegates::OnWorldCleanup.AddUObject(this, &URiveFullScreenUserWidget::OnWorldCleanup);

			RiveFullScreenUserWidgetPrivate::FWorldCleanupListener::Get()->AddWidget(this);
		}
	}
	
	return bWasAdded;
}

void URiveFullScreenUserWidget::Hide()
{
	bDisplayRequested = false;

	if (CurrentDisplayType != ERiveWidgetDisplayType::Inactive)
	{
		ReleaseWidget();

		UWorld* WorldInstance = World.Get();

		if (CurrentDisplayType == ERiveWidgetDisplayType::Viewport)
		{
			ViewportDisplayType.Hide(WorldInstance);
		}
		else if (CurrentDisplayType == ERiveWidgetDisplayType::PostProcessWithBlendMaterial)
		{
			PostProcessDisplayTypeWithBlendMaterial.Hide(WorldInstance);
		}
		else if (CurrentDisplayType == ERiveWidgetDisplayType::PostProcessSceneViewExtension)
		{
			PostProcessWithSceneViewExtensions.Hide(WorldInstance);
		}
		
		CurrentDisplayType = ERiveWidgetDisplayType::Inactive;
	}

	FWorldDelegates::LevelRemovedFromWorld.RemoveAll(this);

	FWorldDelegates::OnWorldCleanup.RemoveAll(this);
	
	RiveFullScreenUserWidgetPrivate::FWorldCleanupListener::Get()->RemoveWidget(this);
	
	World.Reset();
}

void URiveFullScreenUserWidget::Tick(float DeltaSeconds)
{
	if (CurrentDisplayType != ERiveWidgetDisplayType::Inactive)
	{
		UWorld* CurrentWorld = World.Get();

		if (CurrentWorld == nullptr)
		{
			Hide();
		}
		else if (CurrentDisplayType == ERiveWidgetDisplayType::PostProcessWithBlendMaterial)
		{
			PostProcessDisplayTypeWithBlendMaterial.Tick(CurrentWorld, DeltaSeconds);
		}
		else if (CurrentDisplayType == ERiveWidgetDisplayType::PostProcessSceneViewExtension)
		{
			PostProcessWithSceneViewExtensions.Tick(CurrentWorld, DeltaSeconds);
		}
	}
}

void URiveFullScreenUserWidget::SetDisplayTypes(ERiveWidgetDisplayType InEditorDisplayType, ERiveWidgetDisplayType InGameDisplayType, ERiveWidgetDisplayType InPIEDisplayType)
{
	EditorDisplayType = InEditorDisplayType;

	GameDisplayType = InGameDisplayType;

	PIEDisplayType = InPIEDisplayType;
}

void URiveFullScreenUserWidget::SetRiveActor(ARiveActor* InActor)
{
	RiveActor = InActor;
}

void URiveFullScreenUserWidget::SetCustomPostProcessSettingsSource(TWeakObjectPtr<UObject> InCustomPostProcessSettingsSource)
{
	PostProcessDisplayTypeWithBlendMaterial.SetCustomPostProcessSettingsSource(InCustomPostProcessSettingsSource);
}

bool URiveFullScreenUserWidget::ShouldDisplay(UWorld* InWorld) const
{
#if UE_SERVER

	return false;

#else

	if (GUsingNullRHI || HasAnyFlags(RF_ArchetypeObject | RF_ClassDefaultObject) || IsRunningDedicatedServer())
	{
		return false;
	}

	return GetDisplayType(InWorld) != ERiveWidgetDisplayType::Inactive;

#endif // !UE_SERVER
}

ERiveWidgetDisplayType URiveFullScreenUserWidget::GetDisplayType(UWorld* InWorld) const
{
	if (InWorld)
	{
		if (InWorld->WorldType == EWorldType::Game)
		{
			return GameDisplayType;
		}

#if WITH_EDITOR

		else if (InWorld->WorldType == EWorldType::PIE)
		{
			return PIEDisplayType;
		}
		else if (InWorld->WorldType == EWorldType::Editor)
		{
			return EditorDisplayType;
		}

#endif // WITH_EDITOR

	}

	return ERiveWidgetDisplayType::Inactive;
}

bool URiveFullScreenUserWidget::IsDisplayed() const
{
	return CurrentDisplayType != ERiveWidgetDisplayType::Inactive;
}

void URiveFullScreenUserWidget::SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass)
{
	if (ensure(!IsDisplayed()))
	{
		WidgetClass = InWidgetClass;
	}
}

FRiveFullScreenUserWidget_PostProcessBase* URiveFullScreenUserWidget::GetPostProcessDisplayTypeSettingsFor(ERiveWidgetDisplayType Type)
{
	return const_cast<FRiveFullScreenUserWidget_PostProcessBase*>(const_cast<const URiveFullScreenUserWidget*>(this)->GetPostProcessDisplayTypeSettingsFor(Type));
}

const FRiveFullScreenUserWidget_PostProcessBase* URiveFullScreenUserWidget::GetPostProcessDisplayTypeSettingsFor(ERiveWidgetDisplayType Type) const
{
	switch (Type)
	{
		case ERiveWidgetDisplayType::PostProcessWithBlendMaterial:
		{
			return &PostProcessDisplayTypeWithBlendMaterial;
		}
		case ERiveWidgetDisplayType::PostProcessSceneViewExtension:
		{
			return &PostProcessWithSceneViewExtensions;
		}
		case ERiveWidgetDisplayType::Inactive: // Fall-through
		case ERiveWidgetDisplayType::Viewport: 
		{
			ensureMsgf(false, TEXT("GetPostProcessDisplayTypeSettingsFor should only be called with PostProcessWithBlendMaterial or PostProcessSceneViewExtension"));
		
			break;
		}
		default:
			checkNoEntry();
	}

	return nullptr;
}

#if WITH_EDITOR

void URiveFullScreenUserWidget::SetEditorTargetViewport(TWeakPtr<FSceneViewport> InTargetViewport)
{
	EditorTargetViewport = InTargetViewport;

	ViewportDisplayType.EditorTargetViewport = InTargetViewport;

	PostProcessDisplayTypeWithBlendMaterial.EditorTargetViewport = InTargetViewport;

	PostProcessWithSceneViewExtensions.EditorTargetViewport = InTargetViewport;
}

void URiveFullScreenUserWidget::ResetEditorTargetViewport()
{
	EditorTargetViewport.Reset();

	ViewportDisplayType.EditorTargetViewport.Reset();

	PostProcessDisplayTypeWithBlendMaterial.EditorTargetViewport.Reset();

	PostProcessWithSceneViewExtensions.EditorTargetViewport.Reset();
}

#endif // WITH_EDITOR

bool URiveFullScreenUserWidget::InitWidget()
{
	const bool bCanCreate = !Widget && WidgetClass && ensure(World.Get()) && FSlateApplication::IsInitialized();
	
	if (!bCanCreate)
	{
		return false;
	}

	// Could fail e.g. if the class has been marked deprecated or abstract.
	Widget = CreateWidget(World.Get(), WidgetClass);
	
	UE_CLOG(!Widget, LogRive, Warning, TEXT("Failed to create widget with class %s. Review the log for more info."), *WidgetClass->GetPathName())
	
		if (Widget)
	{
		Widget->SetFlags(RF_Transient);
	}

	return Widget != nullptr;
}

void URiveFullScreenUserWidget::ReleaseWidget()
{
	Widget = nullptr;
}

FVector2D URiveFullScreenUserWidget::FindSceneViewportSize()
{
	ensure(World.IsValid());
	
	const UWorld* CurrentWorld = World.Get();
	
	const bool bIsPlayWorld = CurrentWorld && (CurrentWorld->WorldType == EWorldType::Game || CurrentWorld->WorldType == EWorldType::PIE); 
	
	if (bIsPlayWorld)
	{
		if (UGameViewportClient* ViewportClient = World->GetGameViewport())
		{
			FVector2D OutSize;
	
			ViewportClient->GetViewportSize(OutSize);
			
			return OutSize;
		}
	}

#if WITH_EDITOR
	
	if (const TSharedPtr<FSceneViewport> TargetViewportPin = EditorTargetViewport.Pin())
	{
		return TargetViewportPin->GetSize();
	}

#endif // WITH_EDITOR

	ensureMsgf(false, TEXT(
		"FindSceneViewportSize failed. Likely Hide() was called (making World = nullptr) or EditorTargetViewport "
		"reset externally (possibly as part of Hide()). After Hide() is called all widget code should stop calling "
		"FindSceneViewportSize. Investigate whether something was not cleaned up correctly!"
		)
	);

	return FVector2d::ZeroVector;
}

float URiveFullScreenUserWidget::GetViewportDPIScale()
{
	float UIScale = 1.f;
	
	float PlatformScale = FPlatformApplicationMisc::GetDPIScaleFactorAtPoint(10.f, 10.f);

	UWorld* CurrentWorld = World.Get();

	if ((CurrentDisplayType == ERiveWidgetDisplayType::Viewport) && CurrentWorld && (CurrentWorld->WorldType == EWorldType::Game || CurrentWorld->WorldType == EWorldType::PIE))
	{
		// If we are in Game or PIE in Viewport display mode, the GameLayerManager will scale correctly so just return the Platform Scale
		UIScale = PlatformScale;
	}
	else
	{
		// Otherwise when in Editor mode, the editor automatically scales to the platform size, so we only care about the UI scale
		const FIntPoint ViewportSize = FindSceneViewportSize().IntPoint();

		UIScale = GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(ViewportSize);
	}

	return UIScale;
}

void URiveFullScreenUserWidget::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	// If the InLevel is invalid, then the entire world is about to disappear.
	//Hide the widget to clear the memory and reference to the world it may hold.
	if (InLevel == nullptr && InWorld && InWorld == World.Get())
	{
		Hide();
	}
}

void URiveFullScreenUserWidget::OnWorldCleanup(UWorld* InWorld, bool bSessionEnded, bool bCleanupResources)
{
	if (IsDisplayed() && World == InWorld)
	{
		Hide();
	}
}
