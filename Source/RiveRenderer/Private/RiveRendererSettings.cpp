// Fill out your copyright notice in the Description page of Project Settings.


#include "RiveRendererSettings.h"

URiveRendererSettings::URiveRendererSettings() : bEnableRHITechPreview(false)
{
#if WITH_EDITOR
	this->OnSettingChanged().AddLambda([this](UObject*, struct FPropertyChangedEvent&)
	{
		FUnrealEdMisc::Get().RestartEditor();
	});
#endif
}

