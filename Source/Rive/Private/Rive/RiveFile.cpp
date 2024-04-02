// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveFile.h"
#include "IRiveRenderTarget.h"
#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "EditorFramework/AssetImportData.h"
#include "RiveCore/Public/RiveArtboard.h"
#include "Logs/RiveLog.h"
#include "RiveCore/Public/Assets/RiveAsset.h"
#include "RiveCore/Public/Assets/URAssetImporter.h"
#include "RiveCore/Public/Assets/URFileAssetLoader.h"
#include "HAL/FileManager.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/Paths.h"
#include "Async/Async.h"
#include "RenderingThread.h"

#if WITH_RIVE
#include "PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#include "rive/pls/pls_render_context.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

URiveFile::URiveFile()
{
	ArtboardIndex = 0;
}

void URiveFile::BeginDestroy()
{
	InitState = ERiveInitState::Deinitializing;
	
	RiveRenderTarget.Reset();
	
	if (IsValid(Artboard))
	{
		Artboard->MarkAsGarbage();
	}
	
	RiveNativeFileSpan = {};
	RiveNativeFilePtr.reset();
	
	Super::BeginDestroy();
}

TStatId URiveFile::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URiveFile, STATGROUP_Tickables);
}

void URiveFile::Tick(float InDeltaSeconds)
{
	if (!IsValidChecked(this))
	{
		return;
	}

#if WITH_RIVE
	if (IsInitialized() && bIsRendering)
	{
		if (GetArtboard())
		{
			Artboard->Tick(InDeltaSeconds);
			RiveRenderTarget->SubmitAndClear();
		}
	}
#endif // WITH_RIVE
}

bool URiveFile::IsTickable() const
{
	return !HasAnyFlags(RF_ClassDefaultObject) && bIsRendering;
}

void URiveFile::PostLoad()
{
	Super::PostLoad();
	
#if WITH_EDITORONLY_DATA
	// Here we make sure that the AssetImportData matches the RiveFilePath
	if (ensure(AssetImportData))
	{
		AssetImportData->OnImportDataChanged.AddUObject(this, &URiveFile::OnImportDataChanged);
		if (ParentRiveFile)
		{
			RiveFilePath = FString();
			AssetImportData->UpdateFilenameOnly(RiveFilePath);
		}
		else
		{
			const FString Filename = AssetImportData->GetFirstFilename();
			const FString FullPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*Filename);
			if (Filename.IsEmpty()) // for old files
			{
				AssetImportData->UpdateFilenameOnly(RiveFilePath);
			}
			else if (!FPaths::IsSamePath(FullPath, RiveFilePath))
			{
				UE_LOG(LogRive, Warning, TEXT("The path of RiveFile '%s' is not matching the AssetImportData, resetting to RiveFilePath. RiveFilePath: '%s'  AssetImportData: '%s'"),
					*GetFullName(), *RiveFilePath, *FullPath)
				AssetImportData->UpdateFilenameOnly(RiveFilePath);
				// We want to mark the UObject dirty but that is not possible on PostLoad, so we start an AsyncTask
				AsyncTask(ENamedThreads::GameThread, [this]()
				{
					Modify(true);
				});
			}
		}
		AssetImportData->OnImportDataChanged.AddUObject(this, &URiveFile::OnImportDataChanged);
	}
#endif

	if (ParentRiveFile && !ParentRiveFile->OnStartInitializingDelegate.IsBoundToObject(this))
	{
		ParentRiveFile->OnStartInitializingDelegate.AddLambda([this](URiveFile* ParentRive)
		{
			UE_LOG(LogRive, Warning, TEXT("Parent of RiveFile '%s' is starting to initalize (Parent: '%s'). Invalidating Rive File..."), *GetFullName(), *GetFullNameSafe(ParentRive))
			InitState = ERiveInitState::Uninitialized; // to be able to enter the Initialize function
			Initialize(); // This will just set our internal status to initializing, will not finish initializing now
		});
		ParentRiveFile->OnInitializedDelegate.AddLambda([this](URiveFile* ParentRive, bool bSuccess)
		{
			UE_LOG(LogRive, Warning, TEXT("Parent of RiveFile '%s' has just been initialized (Parent: '%s'). Invalidating Rive File..."), *GetFullName(), *GetFullNameSafe(ParentRive))
			InitState = ERiveInitState::Uninitialized; // to be able to enter the Initialize function
			if (bSuccess)
			{
				Initialize(); // we are now ready to initialize
			}
			else
			{
				UE_LOG(LogRive, Error, TEXT("Unable to Initialize this URiveFile Instance '%s' because its Parent '%s' cannot be initialized successfully"), *GetNameSafe(this), *GetNameSafe(ParentRive));
			}
		});
	}
	
	if (!IsRunningCommandlet())
	{
		UE::Rive::Renderer::IRiveRendererModule::Get().CallOrRegister_OnRendererInitialized(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &URiveFile::Initialize));
	}
}

#if WITH_EDITOR

void URiveFile::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const FName ActiveMemberNodeName = *PropertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetValue()->GetName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(URiveFile, ArtboardIndex) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(URiveFile, ArtboardName))
	{
		InstantiateArtboard();
	}
	else if (ActiveMemberNodeName == GET_MEMBER_NAME_CHECKED(URiveFile, Size))
	{
		ResizeRenderTargets(Size);
	}
	else if (ActiveMemberNodeName == GET_MEMBER_NAME_CHECKED(URiveFile, ClearColor))
	{
		if (RiveRenderTarget)
		{
			RiveRenderTarget->SetClearColor(ClearColor);
		}
	}

	// Update the Rive CachedPLSRenderTarget
	if (RiveRenderTarget)
	{
		RiveRenderTarget->Initialize();
	}

	FlushRenderingCommands();
}

#endif // WITH_EDITOR

URiveFile* URiveFile::CreateInstance(const FString& InArtboardName, const FString& InStateMachineName)
{
	auto NewRiveFileInstance = NewObject<
		URiveFile>(this, URiveFile::StaticClass(), NAME_None, RF_Public | RF_Transient);
	NewRiveFileInstance->ParentRiveFile = this;
	NewRiveFileInstance->ArtboardName = InArtboardName.IsEmpty() ? ArtboardName : InArtboardName;
	NewRiveFileInstance->StateMachineName = InStateMachineName.IsEmpty() ? StateMachineName : InStateMachineName;
	NewRiveFileInstance->ArtboardIndex = ArtboardIndex;
	NewRiveFileInstance->PostLoad();
	return NewRiveFileInstance;
}

void URiveFile::FireTrigger(const FString& InPropertyName) const
{
#if WITH_RIVE
	if (GetArtboard())
	{
		GetArtboard()->FireTrigger(InPropertyName);
	}
#endif // WITH_RIVE
}

bool URiveFile::GetBoolValue(const FString& InPropertyName) const
{
#if WITH_RIVE
	if (GetArtboard())
	{
		return GetArtboard()->GetBoolValue(InPropertyName);
	}
#endif // !WITH_RIVE
	return false;
}

float URiveFile::GetNumberValue(const FString& InPropertyName) const
{
#if WITH_RIVE
	if (GetArtboard())
	{
		return GetArtboard()->GetNumberValue(InPropertyName);
	}
#endif // !WITH_RIVE
	return 0.f;
}

FLinearColor URiveFile::GetClearColor() const
{
	return ClearColor;
}

FVector2f URiveFile::GetLocalCoordinate(URiveArtboard* InArtboard, const FVector2f& InPosition)
{
#if WITH_RIVE
	if (InArtboard)
	{
		return InArtboard->GetLocalCoordinate(InPosition, Size, RiveAlignment, RiveFitType);
	}
#endif // WITH_RIVE
	return FVector2f::ZeroVector;
}

FVector2f URiveFile::GetLocalCoordinatesFromExtents(const FVector2f& InPosition, const FBox2f& InExtents) const
{
#if WITH_RIVE
	if (GetArtboard())
	{
		return GetArtboard()->GetLocalCoordinatesFromExtents(InPosition, InExtents, Size, RiveAlignment, RiveFitType);
	}
#endif // WITH_RIVE
	return FVector2f::ZeroVector;
}

void URiveFile::SetBoolValue(const FString& InPropertyName, bool bNewValue)
{
#if WITH_RIVE
	if (GetArtboard())
	{
		Artboard->SetBoolValue(InPropertyName, bNewValue);
	}
#endif // WITH_RIVE
}

void URiveFile::SetNumberValue(const FString& InPropertyName, float NewValue)
{
#if WITH_RIVE
	if (GetArtboard())
	{
		Artboard->SetNumberValue(InPropertyName, NewValue);
	}
#endif // WITH_RIVE
}

#if WITH_EDITOR

bool URiveFile::EditorImport(const FString& InRiveFilePath, TArray<uint8>& InRiveFileBuffer, bool bIsReimport)
{
	if (!UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer())
	{
		UE_LOG(LogRive, Error, TEXT("Unable to Import the RiveFile '%s' as the RiveRenderer is null"), *InRiveFilePath);
		return false;
	}
	bNeedsImport = true;
	RiveFilePath = InRiveFilePath;
	RiveFileData = MoveTemp(InRiveFileBuffer);
	if (bIsReimport)
	{
		Initialize();
	}
	else
	{
		SetFlags(RF_NeedPostLoad);
		ConditionalPostLoad();
	}
	
#if WITH_EDITORONLY_DATA
	if (ensure(AssetImportData))
	{
		AssetImportData->Update(ParentRiveFile ? FString() : InRiveFilePath);
	}
#endif
	
	// In Theory, the import should be synchronous as the IRiveRendererModule::Get().GetRenderer() should have been initialized,
	// and the parent Rive File (if any) should already be loaded
	ensureMsgf(WasLastInitializationSuccessful.IsSet(),
		TEXT("The initialization of the Rive File '%s' ended up being asynchronous. EditorImport returning true as we don't know if the import was successful."),
		*InRiveFilePath);
	
	return WasLastInitializationSuccessful.Get(true);
}

#endif // WITH_EDITOR

void URiveFile::Initialize()
{
	if (!(InitState == ERiveInitState::Uninitialized || (InitState == ERiveInitState::Initialized && bNeedsImport)))
	{
		return;
	}
	
	if (Artboard)
	{
		Artboard->ConditionalBeginDestroy();
		Artboard = nullptr;
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
	
	if (ParentRiveFile)
	{
		if (!ensure(IsValid(ParentRiveFile)))
		{
			UE_LOG(LogRive, Error, TEXT("Unable to Initialize this URiveFile Instance '%s' because its Parent is not valid"), *GetNameSafe(this));
			BroadcastInitializationResult(false);
			return;
		}

		if (!ParentRiveFile->IsInitialized())
		{
			ParentRiveFile->Initialize();
			if (ParentRiveFile->InitState < ERiveInitState::Initializing) 
			{
				UE_LOG(LogRive, Error, TEXT("Unable to Initialize this URiveFile Instance '%s' because its Parent '%s' cannot be initialized"), *GetNameSafe(this), *GetNameSafe(ParentRiveFile));
				BroadcastInitializationResult(false);
			}
			return;
		}
	}
	else if (RiveNativeFileSpan.empty() || bNeedsImport)
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

			if (!ParentRiveFile)
			{
				FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
				rive::ImportResult ImportResult;
				if (bNeedsImport)
				{
					bNeedsImport = false;
					const TUniquePtr<UE::Rive::Assets::FURAssetImporter> AssetImporter = MakeUnique<UE::Rive::Assets::FURAssetImporter>(GetOutermost(), RiveFilePath, GetAssets());
					RiveNativeFilePtr = rive::File::import(RiveNativeFileSpan, PLSRenderContext, &ImportResult, AssetImporter.Get());
					if (ImportResult != rive::ImportResult::success)
					{
						UE_LOG(LogRive, Error, TEXT("Failed to import rive file."));
						Lock.Unlock();
						BroadcastInitializationResult(false);
						return;
					}
				}
				
				const TUniquePtr<UE::Rive::Assets::FURFileAssetLoader> FileAssetLoader = MakeUnique<UE::Rive::Assets::FURFileAssetLoader>(this, GetAssets());
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
			}
			else if (ensure(IsValid(ParentRiveFile)))
			{
				ArtboardNames = ParentRiveFile->ArtboardNames;
			}
			
			InstantiateArtboard(false); // We want the ArtboardChanged event to be raised after the Initialization Result
			if (ensure(GetArtboard()))
			{
				BroadcastInitializationResult(true);
			}
			else
			{
				UE_LOG(LogRive, Error, TEXT("Failed to instantiate the Artboard after importing the rive file."));
				BroadcastInitializationResult(false);
			}
			OnArtboardChangedRaw.Broadcast(this, Artboard);
			OnArtboardChanged.Broadcast(this, Artboard); // Now we can broadcast the Artboard Changed Event
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

void URiveFile::InstantiateArtboard(bool bRaiseArtboardChangedEvent)
{
	Artboard = nullptr;
	
	if (!UE::Rive::Renderer::IRiveRendererModule::IsAvailable())
	{
		UE_LOG(LogRive, Error, TEXT("Could not load rive file as the required Rive Renderer Module is either missing or not loaded properly."));
		return;
	}

	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();

	if (!RiveRenderer)
	{
		UE_LOG(LogRive, Error, TEXT("Failed to instantiate the Artboard of Rive file '%s' as we do not have a valid renderer."), *GetFullNameSafe(this));
		return;
	}

	if (!RiveRenderer->IsInitialized())
	{
		UE_LOG(LogRive, Error, TEXT("Could not load rive file as the required Rive Renderer is not initialized."));
		return;
	}

	if (!GetNativeFile())
	{
		UE_LOG(LogRive, Error, TEXT("Could not instance artboard as our native rive file is invalid."));
		return;
	}
	
	Artboard = NewObject<URiveArtboard>(this);
	
	RiveRenderTarget.Reset();
	RiveRenderTarget = RiveRenderer->CreateTextureTarget_GameThread(GetFName(), this);
	if(!OnResourceInitializedOnRenderThread.IsBoundToObject(this))
	{
		OnResourceInitializedOnRenderThread.AddUObject(this, &URiveFile::OnResourceInitialized_RenderThread);
	}
	RiveRenderTarget->SetClearColor(ClearColor);
	
	if (ArtboardName.IsEmpty())
	{
		Artboard->Initialize(GetNativeFile(), RiveRenderTarget, ArtboardIndex, StateMachineName);
	}
	else
	{
		Artboard->Initialize(GetNativeFile(), RiveRenderTarget, ArtboardName, StateMachineName);
	}

	Artboard->OnArtboardTick_Render.BindDynamic(this, &URiveFile::OnArtboardTickRender);
	Artboard->OnGetLocalCoordinate.BindDynamic(this, &URiveFile::GetLocalCoordinate);

	ResizeRenderTargets(bManualSize ? Size : Artboard->GetSize());
	RiveRenderTarget->Initialize();

	PrintStats();
	
	if (bRaiseArtboardChangedEvent)
	{
		OnArtboardChangedRaw.Broadcast(this, Artboard);
		OnArtboardChanged.Broadcast(this, Artboard);
	}
}

void URiveFile::OnResourceInitialized_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRHIRef& NewResource) const
{
	// When the resource change, we need to tell the Render Target otherwise we will keep on drawing on an outdated RT
	if (const UE::Rive::Renderer::IRiveRenderTargetPtr RenderTarget = RiveRenderTarget) //todo: might need a lock
	{
		RenderTarget->CacheTextureTarget_RenderThread(RHICmdList, NewResource);
	}
}

void URiveFile::OnArtboardTickRender(float DeltaTime, URiveArtboard* InArtboard)
{
	InArtboard->Align(RiveFitType, RiveAlignment);
	InArtboard->Draw();
}

void URiveFile::SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass)
{
	WidgetClass = InWidgetClass;
}

URiveArtboard* URiveFile::GetArtboard() const
{
#if WITH_RIVE
	if (IsValid(Artboard) && Artboard->IsInitialized())
	{
		return Artboard;
	}
#endif // WITH_RIVE
	return nullptr;
}

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

#if WITH_EDITORONLY_DATA
void URiveFile::OnImportDataChanged(const FAssetImportInfo& OldData, const UAssetImportData* NewData)
{
	if (!ParentRiveFile)
	{
		if (NewData)
		{
			RiveFilePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*NewData->GetFirstFilename());
		}
	}
	else
	{
		RiveFilePath = FString();
	}
}
#endif