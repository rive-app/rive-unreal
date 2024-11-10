// Copyright Rive, Inc. All rights reserved.
#pragma once

#include <memory>
#include "Assets/RiveAsset.h"
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "RiveTypes.h"
#include "UObject/Object.h"

#if WITH_RIVE

THIRD_PARTY_INCLUDES_START
#include "rive/file.hpp"
#include "rive/span.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

#include "RiveFile.generated.h"

class URiveAsset;
class URiveArtboard;

/**
 *
 */
UCLASS(BlueprintType, Blueprintable)
class RIVE_API URiveFile : public UObject
{
    GENERATED_BODY()

    friend URiveArtboard;

public:
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnRiveFileInitializationResult,
                                        bool /* bSuccess */);
    DECLARE_MULTICAST_DELEGATE(FOnRiveFileEvent);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRiveReadyDelegate);

    void BeginDestroy() override;
    void PostLoad() override;
    void Initialize();

    void SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass)
    {
        WidgetClass = InWidgetClass;
    }
    TSubclassOf<UUserWidget> GetWidgetClass() const { return WidgetClass; }

    ERiveInitState InitializationState() const { return InitState; }
    UFUNCTION(BlueprintPure, Category = Rive)
    bool IsInitialized() const
    {
        return InitState == ERiveInitState::Initialized;
    }

    /** Delegate called everytime this RiveFile is Initialized */
    FOnRiveFileInitializationResult OnInitializedDelegate;

    /** Delegate called everytime this RiveFile is starting to Initialize */
    FOnRiveFileEvent OnStartInitializingDelegate;

    UPROPERTY(BlueprintAssignable, Category = Rive)
    FRiveReadyDelegate OnRiveReady;

    UPROPERTY(Transient,
              BlueprintReadOnly,
              Category = Rive,
              meta = (NoResetToDefault, AllowPrivateAccess))
    TArray<FString> ArtboardNames;

    // This is only meant as a display feature for the editor, and not meant to
    // be used as functional code
    UPROPERTY(Transient,
              VisibleInstanceOnly,
              Category = Rive,
              NonTransactional,
              meta = (NoResetToDefault, AllowPrivateAccess))
    TArray<URiveArtboard*> Artboards;

    UFUNCTION()
    TArray<FString> GetArtboardNamesForDropdown() const
    {
        return ArtboardNames;
    }

    UPROPERTY(meta = (NoResetToDefault))
    FString RiveFilePath_DEPRECATED;

#if WITH_EDITORONLY_DATA
    // This property holds the import data
    UPROPERTY(VisibleAnywhere, Instanced, Category = "Import Settings")
    UAssetImportData* AssetImportData;
#endif

private:
    void BroadcastInitializationResult(bool bSuccess);
    TOptional<bool> WasLastInitializationSuccessful{};
    FOnRiveFileInitializationResult OnInitializedOnceDelegate;

    UPROPERTY(Transient, meta = (NoResetToDefault))
    ERiveInitState InitState = ERiveInitState::Uninitialized;

    UPROPERTY()
    TArray<uint8> RiveFileData;

    UPROPERTY(VisibleAnywhere, Category = Rive, meta = (NoResetToDefault))
    TSubclassOf<UUserWidget> WidgetClass;

public:
    UPROPERTY(VisibleAnywhere, Category = Rive, meta = (NoResetToDefault))
    TMap<uint32, TObjectPtr<URiveAsset>> Assets;

    TMap<uint32, TObjectPtr<URiveAsset>>& GetAssets() { return Assets; }

    UFUNCTION(BlueprintCallable, Category = Rive)
    URiveAsset* GetRiveAssetById(int32 InId) const
    {
        for (const TTuple<unsigned int, TObjectPtr<URiveAsset>>& x : Assets)
        {
            if (x.Value->Id == InId)
            {
                return x.Value;
            }
        }

        return nullptr;
    }

    UFUNCTION(BlueprintCallable, Category = Rive)
    URiveAsset* GetRiveAssetByName(FString InName) const
    {
        for (const TTuple<unsigned int, TObjectPtr<URiveAsset>>& x : Assets)
        {
            if (x.Value->Name.Equals(InName))
            {
                return x.Value;
            }
        }

        return nullptr;
    }

    rive::Span<const uint8> RiveNativeFileSpan;
    rive::Span<const uint8>& GetNativeFileSpan() { return RiveNativeFileSpan; }

    std::unique_ptr<rive::File> RiveNativeFilePtr;

    rive::File* GetNativeFile() const
    {
        if (RiveNativeFilePtr)
        {
            return RiveNativeFilePtr.get();
        }

        return nullptr;
    }

    void PrintStats() const;

#if WITH_EDITOR

    bool EditorImport(const FString& InRiveFilePath,
                      TArray<uint8>& InRiveFileBuffer,
                      bool bIsReimport = false);

#endif // WITH_EDITOR

    bool bNeedsImport = false;
};
