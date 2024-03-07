// Copyright Rive, Inc. All rights reserved.

#include "RiveEditorModule.h"

#include "RiveTextureThumbnailRenderer.h"
#include "Rive/RiveFile.h"
#include "ThumbnailRendering/ThumbnailManager.h"

#define LOCTEXT_NAMESPACE "RiveEditorModule"

void UE::Rive::Editor::Private::FRiveEditorModuleModule::StartupModule()
{
	UThumbnailManager::Get().RegisterCustomRenderer(URiveFile::StaticClass(), URiveTextureThumbnailRenderer::StaticClass());
}

void UE::Rive::Editor::Private::FRiveEditorModuleModule::ShutdownModule()
{
	if (UObjectInitialized())
	{
		UThumbnailManager::Get().UnregisterCustomRenderer(URiveFile::StaticClass());
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(UE::Rive::Editor::Private::FRiveEditorModuleModule, RiveEditorModule)
