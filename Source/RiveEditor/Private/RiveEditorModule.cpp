// Copyright Epic Games, Inc. All Rights Reserved.

#include "RiveEditorModule.h"

#define LOCTEXT_NAMESPACE "RiveEditorModule"

void UE::Rive::Editor::Private::FRiveEditorModuleModule::StartupModule()
{
}

void UE::Rive::Editor::Private::FRiveEditorModuleModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(UE::Rive::Editor::Private::FRiveEditorModuleModule, RiveEditorModule)
