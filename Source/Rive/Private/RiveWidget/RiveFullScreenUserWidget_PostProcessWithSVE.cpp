// Copyright Epic Games, Inc. All Rights Reserved.

#include "RiveWidget/RiveFullScreenUserWidget_PostProcessWithSVE.h"

#include "RivePostProcessSceneViewExtension.h"
#include "SceneViewExtension.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "RiveWidget/RiveFullScreenUserWidget.h"

bool FRiveFullScreenUserWidget_PostProcessWithSVE::Display(UWorld* World, UUserWidget* Widget, TAttribute<float> InDPIScale)
{
	bool bOk = CreateRenderer(World, Widget, MoveTemp(InDPIScale));
	if (bOk && ensureMsgf(WidgetRenderTarget, TEXT("CreateRenderer returned true even though it failed.")))
	{
		SceneViewExtension = FSceneViewExtensions::NewExtension<FRivePostProcessSceneViewExtension>(
			*WidgetRenderTarget
			);
		SceneViewExtension->IsActiveThisFrameFunctions = MoveTemp(IsActiveFunctorsToRegister);
	}
	return bOk;
}

void FRiveFullScreenUserWidget_PostProcessWithSVE::Hide(UWorld* World)
{
	SceneViewExtension.Reset();
	FRiveFullScreenUserWidget_PostProcessBase::Hide(World);
}

void FRiveFullScreenUserWidget_PostProcessWithSVE::Tick(UWorld* World, float DeltaSeconds)
{
	TickRenderer(World, DeltaSeconds);
}

void FRiveFullScreenUserWidget_PostProcessWithSVE::RegisterIsActiveFunctor(FSceneViewExtensionIsActiveFunctor IsActiveFunctor)
{
	if (SceneViewExtension)
	{
		SceneViewExtension->IsActiveThisFrameFunctions.Emplace(MoveTemp(IsActiveFunctor));
	}
	else
	{
		IsActiveFunctorsToRegister.Emplace(MoveTemp(IsActiveFunctor));
	}
}
