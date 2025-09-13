// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#pragma once
#include "MatrixTypes.h"
#include "RiveAudioEngine.h"
#include "RiveInternalTypes.h"
#include "RiveTypes.h"
#include "RiveStateMachine.h"
#include "rive/command_queue.hpp"
#include "Input/Events.h"
#include "Layout/Geometry.h"
#include "Tickable.h"

#if WITH_RIVE
struct FArtboardDefinition;
struct FRiveCommandBuilder;
class FRiveRenderTarget;
class URiveFile;
class URiveTextureObject;

THIRD_PARTY_INCLUDES_START
#include "rive/file.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

#include "RiveArtboard.generated.h"

UCLASS(BlueprintType)
class RIVE_API URiveArtboard : public UObject, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void BeginDestroy() override;

    virtual bool IsTickableInEditor() const override { return true; }

    virtual UWorld* GetTickableGameObjectWorld() const override
    {
        return GetWorld();
    }

    virtual TStatId GetStatId() const override;

    virtual void Tick(float InDeltaSeconds) override;
    virtual bool IsTickable() const override;

    DECLARE_MULTICAST_DELEGATE_OneParam(FDataReadyDelegate, URiveArtboard*);
    FDataReadyDelegate OnDataReady;

    /** Returns the transformation Matrix from the start of the Render Queue up
     * to now */
    UFUNCTION(BlueprintCallable, Category = Rive)
    FMatrix GetLastDrawTransformMatrix() const { return LastDrawTransform; }

    void DrawToRenderTarget(FRiveCommandBuilder& CommandBuilder,
                            TSharedPtr<FRiveRenderTarget> RenderTarget);

    /*
     * This requires that the audio engine has been initialized via BeginPlay
     * before setting
     */
    UFUNCTION(BlueprintCallable, Category = Rive)
    void SetAudioEngine(URiveAudioEngine* AudioEngine);
#if WITH_EDITORONLY_DATA
    void InitializeForDataImport(URiveFile* InRiveFile,
                                 const FString& Name,
                                 FRiveCommandBuilder& InCommandBuilder);
#endif
    void Initialize(URiveFile* InRiveFile,
                    const FArtboardDefinition& InDefinition,
                    bool InAutoBindViewModel,
                    FRiveCommandBuilder& InCommandBuilder);

    void Initialize(URiveFile* InRiveFile,
                    const FArtboardDefinition& InDefinition,
                    const FString& InStateMachineName,
                    bool InAutoBindViewModel,
                    FRiveCommandBuilder& InCommandBuilder);

    void ErrorReceived(uint64_t RequestId);

    UFUNCTION(BlueprintCallable, Category = "Rive|Artboard")
    void SetStateMachine(const FString& StateMachineName,
                         bool InAutoBindViewModel);

    UFUNCTION(BlueprintCallable, Category = "Rive|Artboard")
    void SetViewModel(URiveViewModel* RiveViewModelInstance);

    TArray<FString> GetStateMachineNames() const { return StateMachineNames; }
    FString GetDefaultViewModel() const { return DefaultViewModelName; }
    FString GetDefaultViewModelInstance() const
    {
        return DefaultViewModelInstanceName;
    }

    const FString& GetArtboardName() const { return ArtboardDefinition.Name; }
    rive::ArtboardHandle GetNativeArtboardHandle() const
    {
        return NativeArtboardHandle;
    }

    bool PointerDown(const FGeometry& MyGeometry,
                     const FRiveDescriptor& InDescriptor,
                     const FPointerEvent& MouseEvent)
    {
        if (!StateMachine.IsValid())
            return false;
        return StateMachine->PointerDown(MyGeometry, InDescriptor, MouseEvent);
    }

    bool PointerMove(const FGeometry& MyGeometry,
                     const FRiveDescriptor& InDescriptor,
                     const FPointerEvent& MouseEvent)
    {
        if (!StateMachine.IsValid())
            return false;
        return StateMachine->PointerMove(MyGeometry, InDescriptor, MouseEvent);
    }

    bool PointerUp(const FGeometry& MyGeometry,
                   const FRiveDescriptor& InDescriptor,
                   const FPointerEvent& MouseEvent)
    {
        if (!StateMachine.IsValid())
            return false;
        return StateMachine->PointerUp(MyGeometry, InDescriptor, MouseEvent);
    }

    bool PointerExit(const FGeometry& InGeometry,
                     const FRiveDescriptor& InDescriptor,
                     const FPointerEvent& MouseEvent)
    {
        if (!StateMachine.IsValid())
            return false;
        return StateMachine->PointerExit(InGeometry, InDescriptor, MouseEvent);
    }

    // This is the size of the artboard in the Rive file. It is not the size of
    // the rendered artboard.
    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "RiveFileData")
    FVector2D ArtboardDefaultSize;

#if WITH_EDITORONLY_DATA
    void StateMachinesListed(std::vector<std::string> Names);
    void DefaultViewModelInfoReceived(std::string ViewModel,
                                      std::string ViewModelInstance);

    bool HasData() const
    {
        return bHasStateMachineNames && bHasDefaultViewModelInfo &&
               bHasDefaultViewModel && bHasDefaultArtboardSize;
    }
#else
    bool HasData() const { return true; }
#endif

private:
    void SetupStateMachine(const FString& StateMachineName,
                           bool InAutoBindViewModel);
    void SetupStateMachine(FRiveCommandBuilder& Builder,
                           const FString& StateMachineName,
                           bool InAutoBindViewModel);

    rive::ArtboardHandle NativeArtboardHandle = RIVE_NULL_HANDLE;
    TSharedPtr<FRiveStateMachine> StateMachine = nullptr;

    TWeakObjectPtr<URiveFile> RiveFile;

    UPROPERTY(Transient,
              VisibleInstanceOnly,
              BlueprintReadOnly,
              Category = Rive,
              meta = (NoResetToDefault, AllowPrivateAccess))
    FArtboardDefinition ArtboardDefinition;

    UPROPERTY(Transient,
              VisibleInstanceOnly,
              BlueprintReadOnly,
              Category = Rive,
              meta = (NoResetToDefault, AllowPrivateAccess))
    TArray<FString> StateMachineNames;

    UPROPERTY(Transient,
              VisibleInstanceOnly,
              BlueprintReadOnly,
              Category = Rive,
              meta = (NoResetToDefault, AllowPrivateAccess))
    FString DefaultViewModelName;

    UPROPERTY(Transient,
              VisibleInstanceOnly,
              BlueprintReadOnly,
              Category = Rive,
              meta = (NoResetToDefault, AllowPrivateAccess))
    FString DefaultViewModelInstanceName;

    UPROPERTY(Transient)
    TObjectPtr<URiveViewModel> BoundViewModel;

#if WITH_EDITORONLY_DATA
    void BroadcastHasDataIfReady()
    {
        if (bHasStateMachineNames && bHasDefaultViewModelInfo &&
            bHasDefaultArtboardSize)
        {
            OnDataReady.Broadcast(this);
        }
    }

    bool bHasStateMachineNames = false;
    bool bHasDefaultViewModelInfo = false;
    bool bHasDefaultViewModel = false;
    bool bHasDefaultArtboardSize = false;

    uint64_t GetDefaultViewModelRequestId = 0;
#endif

    /** The Matrix at the time of the last call to Draw for this Artboard **/
    FMatrix LastDrawTransform = FMatrix::Identity;
};
