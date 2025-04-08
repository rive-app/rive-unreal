// Copyright Rive, Inc. All rights reserved.
#include "Rive/RiveFile.h"

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "Logs/RiveLog.h"
#include "Rive/Assets/RiveFileAssetImporter.h"
#include "Rive/Assets/RiveFileAssetLoader.h"
#include "Rive/ViewModel/RiveViewModel.h"
#include "Rive/RiveArtboard.h"
#include "Blueprint/UserWidget.h"
#include "Rive/RiveUtils.h"

#if WITH_EDITOR
#include "EditorFramework/AssetImportData.h"
#endif

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/animation/state_machine_input.hpp"
#include "rive/renderer/render_context.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

class FRiveFileAssetImporter;
class FRiveFileAssetLoader;

void URiveFile::BeginDestroy()
{
    InitState = ERiveInitState::Deinitializing;
    RiveNativeFileSpan = {};
    Super::BeginDestroy();
}

void URiveFile::PostLoad()
{
    UObject::PostLoad();

    if (!IsRunningCommandlet())
    {
        IRiveRendererModule::Get().CallOrRegister_OnRendererInitialized(
            FSimpleMulticastDelegate::FDelegate::CreateUObject(
                this,
                &URiveFile::Initialize));
    }

#if WITH_EDITORONLY_DATA
    if (AssetImportData == nullptr)
    {
        AssetImportData =
            NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
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
    if (!(InitState == ERiveInitState::Uninitialized ||
          (InitState == ERiveInitState::Initialized && bNeedsImport)))
    {
        return;
    }

    WasLastInitializationSuccessful.Reset();
    InitState = ERiveInitState::Initializing;
    OnStartInitializingDelegate.Broadcast();

    if (!IRiveRendererModule::IsAvailable())
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Could not load rive file as the required Rive Renderer "
                    "Module is either "
                    "missing or not loaded properly."));
        BroadcastInitializationResult(false);
        return;
    }

    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!ensure(RiveRenderer))
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to import rive file as we do not have a valid "
                    "renderer."));
        BroadcastInitializationResult(false);
        return;
    }

#if WITH_RIVE
    if (RiveNativeFileSpan.empty() || bNeedsImport)
    {
        if (RiveFileData.IsEmpty())
        {
            UE_LOG(LogRive,
                   Error,
                   TEXT("Could not load an empty Rive File Data."));
            BroadcastInitializationResult(false);
            return;
        }
        RiveNativeFileSpan =
            rive::make_span(RiveFileData.GetData(), RiveFileData.Num());
    }

    InitState = ERiveInitState::Initializing;

    RiveRenderer->CallOrRegister_OnInitialized(
        IRiveRenderer::FOnRendererInitialized::FDelegate::CreateLambda(
            [this](IRiveRenderer* RiveRenderer) {
                rive::gpu::RenderContext* RenderContext;
                {
                    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
                    RenderContext = RiveRenderer->GetRenderContext();
                }

                if (ensure(RenderContext))
                {
                    ArtboardNames.Empty();
                    Artboards.Empty();
                    ViewModels.Empty();

                    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
                    rive::ImportResult ImportResult;

#if WITH_EDITORONLY_DATA
                    if (bNeedsImport)
                    {
                        bNeedsImport = false;
                        const TUniquePtr<FRiveFileAssetImporter> AssetImporter =
                            MakeUnique<FRiveFileAssetImporter>(
                                GetOutermost(),
                                AssetImportData->GetFirstFilename(),
                                GetAssets());
                        RiveNativeFilePtr =
                            rive::File::import(RiveNativeFileSpan,
                                               RenderContext,
                                               &ImportResult,
                                               AssetImporter.Get());
                        if (ImportResult != rive::ImportResult::success)
                        {
                            UE_LOG(LogRive,
                                   Error,
                                   TEXT("Failed to import rive file."));
                            Lock.Unlock();
                            BroadcastInitializationResult(false);
                            return;
                        }
                    }
#endif

                    const TUniquePtr<FRiveFileAssetLoader> FileAssetLoader =
                        MakeUnique<FRiveFileAssetLoader>(this, Assets);
                    RiveNativeFilePtr =
                        rive::File::import(RiveNativeFileSpan,
                                           RenderContext,
                                           &ImportResult,
                                           FileAssetLoader.Get());

                    if (ImportResult != rive::ImportResult::success)
                    {
                        UE_LOG(LogRive,
                               Error,
                               TEXT("Failed to load rive file."));
                        Lock.Unlock();
                        BroadcastInitializationResult(false);
                        return;
                    }

                    // UI Helpers
                    for (int i = 0; i < RiveNativeFilePtr->artboardCount(); ++i)
                    {
                        auto Artboard = NewObject<URiveArtboard>();

                        // We won't tick the artboard, it's just
                        // initialized for informational purposes.
                        Artboard->Initialize(this, nullptr, i, "");
                        Artboards.Add(Artboard);
                        ArtboardNames.Add(Artboard->GetArtboardName());
                    }

                    for (int i = 0; i < RiveNativeFilePtr->viewModelCount();
                         ++i)
                    {
                        auto ViewModel = NewObject<URiveViewModel>();
                        ViewModel->Initialize(
                            RiveNativeFilePtr->viewModelByIndex(i));
                        ViewModels.Add(ViewModel);
                    }

                    BroadcastInitializationResult(true);
                    return;
                }

                UE_LOG(LogRive, Error, TEXT("Failed to import rive file."));
                BroadcastInitializationResult(false);
            }));
#endif // WITH_RIVE
}

void URiveFile::BroadcastInitializationResult(bool bSuccess)
{
    WasLastInitializationSuccessful = bSuccess;
    InitState =
        bSuccess ? ERiveInitState::Initialized : ERiveInitState::Uninitialized;
    // First broadcast the one time fire delegate
    OnInitializedOnceDelegate.Broadcast(bSuccess);
    OnInitializedOnceDelegate.Clear();
    OnInitializedDelegate.Broadcast(bSuccess);
    if (bSuccess)
    {
        OnRiveReady.Broadcast();
    }
}

int32 URiveFile::GetViewModelCount() const
{
    if (!RiveNativeFilePtr)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveFile::GetViewModelCount "
                    "RiveNativeFilePtr is Null."));

        return 0;
    }

    return static_cast<int32>(RiveNativeFilePtr->viewModelCount());
}

URiveViewModel* URiveFile::CreateViewModelWrapper(
    rive::ViewModelRuntime* ViewModel) const
{
    if (!ViewModel)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveFile::CreateViewModelWrapper "
                    "ViewModel is Null."));

        return nullptr;
    }

    URiveViewModel* ViewModelWrapper = NewObject<URiveViewModel>();
    ViewModelWrapper->Initialize(ViewModel);
    return ViewModelWrapper;
}

URiveViewModel* URiveFile::GetViewModelByIndex(int32 Index) const
{
    if (!RiveNativeFilePtr)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveFile::GetViewModelByIndex "
                    "RiveNativeFilePtr is Null."));
        return nullptr;
    }

    if (Index < 0 || Index >= GetViewModelCount())
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveFile::GetViewModelByIndex Index "
                    "must be between 0 and %d."),
               GetViewModelCount() - 1);

        return nullptr;
    }

    return CreateViewModelWrapper(RiveNativeFilePtr->viewModelByIndex(Index));
}

URiveViewModel* URiveFile::GetViewModelByName(const FString& Name) const
{
    if (!RiveNativeFilePtr)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveFile::GetViewModelByName "
                    "RiveNativeFilePtr is Null."));

        return nullptr;
    }

    return CreateViewModelWrapper(
        RiveNativeFilePtr->viewModelByName(RiveUtils::ToUTF8(*Name)));
}

URiveViewModel* URiveFile::GetDefaultArtboardViewModel(
    URiveArtboard* Artboard) const
{
    if (!RiveNativeFilePtr || !Artboard)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveFile::GetDefaultArtboardViewModel "
                    "RiveNativeFilePtr or Artboard is Null."));

        return nullptr;
    }

    rive::Artboard* NativeArtboard = Artboard->GetNativeArtboard();
    if (!NativeArtboard)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveFile::GetDefaultArtboardViewModel "
                    "NativeArtboard is Null."));

        return nullptr;
    }

    return CreateViewModelWrapper(
        RiveNativeFilePtr->defaultArtboardViewModel(NativeArtboard));
}

#if WITH_EDITOR
void URiveFile::PrintStats() const
{
    const rive::File* NativeFile = GetNativeFile();
    if (!NativeFile)
    {
        UE_LOG(LogRive,
               Warning,
               TEXT("Could not print statistics as we have detected an empty "
                    "rive file."));
        return;
    }

    FFormatNamedArguments RiveFileLoadArgs;
    RiveFileLoadArgs.Add(
        TEXT("Major"),
        FText::AsNumber(static_cast<int>(NativeFile->majorVersion)));
    RiveFileLoadArgs.Add(
        TEXT("Minor"),
        FText::AsNumber(static_cast<int>(NativeFile->minorVersion)));
    RiveFileLoadArgs.Add(
        TEXT("NumArtboards"),
        FText::AsNumber(static_cast<uint32>(NativeFile->artboardCount())));
    RiveFileLoadArgs.Add(
        TEXT("NumAssets"),
        FText::AsNumber(static_cast<uint32>(NativeFile->assets().size())));

    if (const rive::Artboard* NativeArtboard = NativeFile->artboard())
    {
        RiveFileLoadArgs.Add(TEXT("NumAnimations"),
                             FText::AsNumber(static_cast<uint32>(
                                 NativeArtboard->animationCount())));
    }
    else
    {
        RiveFileLoadArgs.Add(TEXT("NumAnimations"), FText::AsNumber(0));
    }

    const FText RiveFileLoadMsg =
        FText::Format(NSLOCTEXT("FURFile",
                                "RiveFileLoadMsg",
                                "Using Rive Runtime : {Major}.{Minor}; "
                                "Artboard(s) Count : {NumArtboards}; "
                                "Asset(s) Count : {NumAssets}; Animation(s) "
                                "Count : {NumAnimations}"),
                      RiveFileLoadArgs);

    UE_LOG(LogRive, Display, TEXT("%s"), *RiveFileLoadMsg.ToString());
}

bool URiveFile::EditorImport(const FString& InRiveFilePath,
                             TArray<uint8>& InRiveFileBuffer,
                             bool bIsReimport)
{
    if (!IRiveRendererModule::Get().GetRenderer())
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Unable to Import the RiveFile '%s' as the RiveRenderer is"
                    " null"),
               *InRiveFilePath);
        return false;
    }
    bNeedsImport = true;

#if WITH_EDITORONLY_DATA
    if (AssetImportData == nullptr)
    {
        AssetImportData =
            NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
    }

    AssetImportData->Update(InRiveFilePath);

#endif

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

    // In Theory, the import should be synchronous as the
    // IRiveRendererModule::Get().GetRenderer() should have been initialized,
    // and the parent Rive File (if any) should already be loaded
    ensureMsgf(WasLastInitializationSuccessful.IsSet(),
               TEXT("The initialization of the Rive File '%s' ended up being "
                    "asynchronous. "
                    "EditorImport returning true as we don't know if the "
                    "import was successful."),
               *InRiveFilePath);

    return WasLastInitializationSuccessful.Get(true);
}
#endif // WITH_EDITOR
