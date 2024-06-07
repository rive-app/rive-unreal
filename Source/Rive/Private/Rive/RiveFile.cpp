// Fill out your copyright notice in the Description page of Project Settings.
#include "Rive/RiveFile.h"

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "RiveArtboard.h"
#include "Assets/URAssetImporter.h"
#include "Assets/URFileAssetLoader.h"
#include "Logs/RiveLog.h"

#if WITH_EDITOR
#include "EditorFramework/AssetImportData.h"
#endif

#if WITH_RIVE
#include "PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#include "rive/pls/pls_render_context.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

namespace UE::Rive::Assets
{
	class FURAssetImporter;
	class FURFileAssetLoader;
}

void URiveFile::BeginDestroy()
{
	InitState = ERiveInitState::Deinitializing;
	Artboards.Reset();
	
	RiveNativeFileSpan = {};
	RiveNativeFilePtr.reset();
	UObject::BeginDestroy();
}

void URiveFile::PostLoad()
{
	UObject::PostLoad();
	
	if (!IsRunningCommandlet())
	{
		UE::Rive::Renderer::IRiveRendererModule::Get().CallOrRegister_OnRendererInitialized(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &URiveFile::Initialize));
	}

#if WITH_EDITORONLY_DATA
	if (AssetImportData == nullptr)
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}

	if (!RiveFilePath_DEPRECATED.IsEmpty())
	{
		FAssetImportInfo Info;
		Info.Insert(FAssetImportInfo::FSourceFile(RiveFilePath_DEPRECATED));
		AssetImportData->SourceData = MoveTemp(Info);
	}

#endif
}

void URiveFile::Initialize()
{
	if (!(InitState == ERiveInitState::Uninitialized || (InitState == ERiveInitState::Initialized && bNeedsImport)))
	{
		return;
	}
	
	WasLastInitializationSuccessful.Reset();
	InitState = ERiveInitState::Initializing;
	OnStartInitializingDelegate.Broadcast(this);
	
	if (!UE::Rive::Renderer::IRiveRendererModule::IsAvailable())
	{
		UE_LOG(LogRive, Error, TEXT("Could not load rive file as the required Rive Renderer Module is either missing or not loaded properly."));
		BroadcastInitializationResult(false);
		return;
	}

	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	if (!ensure(RiveRenderer))
	{
		UE_LOG(LogRive, Error, TEXT("Failed to import rive file as we do not have a valid renderer."));
		BroadcastInitializationResult(false);
		return;
	}

#if WITH_RIVE
	if (RiveNativeFileSpan.empty() || bNeedsImport)
	{
		if (RiveFileData.IsEmpty())
		{
			UE_LOG(LogRive, Error, TEXT("Could not load an empty Rive File Data."));
			BroadcastInitializationResult(false);
			return;
		}
		RiveNativeFileSpan = rive::make_span(RiveFileData.GetData(), RiveFileData.Num());
	}

	InitState = ERiveInitState::Initializing;
	
	RiveRenderer->CallOrRegister_OnInitialized(UE::Rive::Renderer::IRiveRenderer::FOnRendererInitialized::FDelegate::CreateLambda(
	[this](UE::Rive::Renderer::IRiveRenderer* RiveRenderer)
	{
		rive::pls::PLSRenderContext* PLSRenderContext;
		{
			FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
			PLSRenderContext = RiveRenderer->GetPLSRenderContextPtr();
		}
		
		if (ensure(PLSRenderContext))
		{
			ArtboardNames.Empty();
			
			FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
			rive::ImportResult ImportResult;

#if WITH_EDITORONLY_DATA
			if (bNeedsImport)
			{
				bNeedsImport = false;
				const TUniquePtr<UE::Rive::Assets::FURAssetImporter> AssetImporter = MakeUnique<UE::Rive::Assets::FURAssetImporter>(GetOutermost(), AssetImportData->GetFirstFilename(), GetAssets());
				RiveNativeFilePtr = rive::File::import(RiveNativeFileSpan, PLSRenderContext, &ImportResult, AssetImporter.Get());
				if (ImportResult != rive::ImportResult::success)
				{
					UE_LOG(LogRive, Error, TEXT("Failed to import rive file."));
					Lock.Unlock();
					BroadcastInitializationResult(false);
					return;
				}
			}
#endif
			
			const TUniquePtr<UE::Rive::Assets::FURFileAssetLoader> FileAssetLoader = MakeUnique<UE::Rive::Assets::FURFileAssetLoader>(this, Assets);
			RiveNativeFilePtr = rive::File::import(RiveNativeFileSpan, PLSRenderContext, &ImportResult, FileAssetLoader.Get());

			if (ImportResult != rive::ImportResult::success)
			{
				UE_LOG(LogRive, Error, TEXT("Failed to load rive file."));
				Lock.Unlock();
				BroadcastInitializationResult(false);
				return;
			}
			
			// UI Helper
			for (int i = 0; i < RiveNativeFilePtr->artboardCount(); ++i)
			{
				rive::Artboard* NativeArtboard = RiveNativeFilePtr->artboard(i);
				ArtboardNames.Add(NativeArtboard->name().c_str());
			}

			for (auto i = Artboards.Num() - 1; i >= 0; i--)
			{
				URiveArtboard* Artboard = Artboards[i];
				Artboard->Reinitialize(RiveNativeFilePtr.get());
			}
			
			BroadcastInitializationResult(true);
			return;
		}

		UE_LOG(LogRive, Error, TEXT("Failed to import rive file."));
		BroadcastInitializationResult(false);
	}));
#endif // WITH_RIVE
}

void URiveFile::WhenInitialized(FOnRiveFileInitialized::FDelegate&& Delegate)
{
	if (WasLastInitializationSuccessful.IsSet())
	{
		Delegate.Execute(this, WasLastInitializationSuccessful.GetValue());
	}
	else
	{
		OnInitializedOnceDelegate.Add(MoveTemp(Delegate));
	}
}

void URiveFile::BroadcastInitializationResult(bool bSuccess)
{
	WasLastInitializationSuccessful = bSuccess;
	InitState = bSuccess ? ERiveInitState::Initialized : ERiveInitState::Uninitialized;
	// First broadcast the one time fire delegate
	OnInitializedOnceDelegate.Broadcast(this, bSuccess);
	OnInitializedOnceDelegate.Clear();
	OnInitializedDelegate.Broadcast(this, bSuccess);
	if (bSuccess)
	{
		OnRiveReady.Broadcast();
	}
}

#if WITH_EDITOR
void URiveFile::PrintStats() const
{
	const rive::File* NativeFile = GetNativeFile();
	if (!NativeFile)
	{
		UE_LOG(LogRive, Error, TEXT("Could not print statistics as we have detected an empty rive file."));
		return;
	}

	FFormatNamedArguments RiveFileLoadArgs;
	RiveFileLoadArgs.Add(TEXT("Major"), FText::AsNumber(static_cast<int>(NativeFile->majorVersion)));
	RiveFileLoadArgs.Add(TEXT("Minor"), FText::AsNumber(static_cast<int>(NativeFile->minorVersion)));
	RiveFileLoadArgs.Add(TEXT("NumArtboards"), FText::AsNumber(static_cast<uint32>(NativeFile->artboardCount())));
	RiveFileLoadArgs.Add(TEXT("NumAssets"), FText::AsNumber(static_cast<uint32>(NativeFile->assets().size())));

	if (const rive::Artboard* NativeArtboard = NativeFile->artboard())
	{
		RiveFileLoadArgs.Add(
			TEXT("NumAnimations"), FText::AsNumber(static_cast<uint32>(NativeArtboard->animationCount())));
	}
	else
	{
		RiveFileLoadArgs.Add(TEXT("NumAnimations"), FText::AsNumber(0));
	}

	const FText RiveFileLoadMsg = FText::Format(NSLOCTEXT("FURFile", "RiveFileLoadMsg",
														  "Using Rive Runtime : {Major}.{Minor}; Artboard(s) Count : {NumArtboards}; Asset(s) Count : {NumAssets}; Animation(s) Count : {NumAnimations}"), RiveFileLoadArgs);

	UE_LOG(LogRive, Display, TEXT("%s"), *RiveFileLoadMsg.ToString());
}

bool URiveFile::EditorImport(const FString& InRiveFilePath, TArray<uint8>& InRiveFileBuffer, bool bIsReimport)
{
	if (!UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer())
	{
		UE_LOG(LogRive, Error, TEXT("Unable to Import the RiveFile '%s' as the RiveRenderer is null"), *InRiveFilePath);
		return false;
	}
	bNeedsImport = true;

#if WITH_EDITORONLY_DATA
	if (AssetImportData == nullptr)
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}

	AssetImportData->Update(InRiveFilePath);

#endif
	
	RiveFileData = MoveTemp(InRiveFileBuffer);
	if (bIsReimport)
	{
		for (auto i = Artboards.Num() - 1; i >= 0; i--)
		{
			URiveArtboard* Artboard = Artboards[i];
			Artboard->Deinitialize();
		}
		
		Initialize();
	}
	else
	{
		SetFlags(RF_NeedPostLoad);
		ConditionalPostLoad();
	}
	
	// In Theory, the import should be synchronous as the IRiveRendererModule::Get().GetRenderer() should have been initialized,
	// and the parent Rive File (if any) should already be loaded
	ensureMsgf(WasLastInitializationSuccessful.IsSet(),
		TEXT("The initialization of the Rive File '%s' ended up being asynchronous. EditorImport returning true as we don't know if the import was successful."),
		*InRiveFilePath);
	
	return WasLastInitializationSuccessful.Get(true);
}
#endif // WITH_EDITOR
