// Copyright Rive, Inc. All rights reserved.
#pragma once

#include <memory>

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RiveTypes.h"

#if WITH_RIVE
#include "PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#include "rive/span.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

#include "RiveFile.generated.h"

class URiveAsset;
class URiveArtboard;

namespace rive
{
	class File;
}

/**
 *
 */
UCLASS(BlueprintType, Blueprintable)
class RIVE_API URiveFile : public UObject
{
	GENERATED_BODY()

	friend URiveArtboard;
public:
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRiveFileInitialized, URiveFile*, bool /* bSuccess */ );
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnRiveFileEvent, URiveFile*);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRiveReadyDelegate);
	
	void BeginDestroy() override;
	void PostLoad() override;
	void Initialize();
	
	void SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass) { WidgetClass = InWidgetClass; }
	TSubclassOf<UUserWidget> GetWidgetClass() const { return WidgetClass; }
	
	ERiveInitState InitializationState() const { return InitState; }
	UFUNCTION(BlueprintPure, Category = Rive)
	bool IsInitialized() const { return InitState == ERiveInitState::Initialized; }

	/**
	 * Call the given delegate once this RiveFile has been initialized.
	 * It is already initialized, the delegate will fire instantly.
	 * The delegate will be called only once.
	 */
	virtual void WhenInitialized(FOnRiveFileInitialized::FDelegate&& Delegate);
	/** Delegate called everytime this RiveFile is Initialized */
	FOnRiveFileInitialized OnInitializedDelegate;
	/** Delegate called everytime this RiveFile is starting to Initialize */
	FOnRiveFileEvent OnStartInitializingDelegate;

	UPROPERTY(BlueprintAssignable, Category = Rive)
	FRiveReadyDelegate OnRiveReady;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category=Rive, meta=(NoResetToDefault, AllowPrivateAccess))
	TArray<FString> ArtboardNames;

	UFUNCTION()
	TArray<FString> GetArtboardNamesForDropdown() const
	{
		TArray<FString> Names {FString{}};
		Names.Append(ArtboardNames);
		return Names;
	}
	
	UFUNCTION()
	TArray<FString> GetStateMachineNamesForDropdown() const
	{
		// return GetNativeFile() ? Artboard->GetStateMachineNamesForDropdown() : TArray<FString>{};
		return {};
	}
	
	UPROPERTY(meta=(NoResetToDefault))
	FString RiveFilePath_DEPRECATED;

#if WITH_EDITORONLY_DATA
	// This property holds the import data
	UPROPERTY(VisibleAnywhere, Instanced, Category = "Import Settings")
	UAssetImportData* AssetImportData;
#endif
	
private:
	void BroadcastInitializationResult(bool bSuccess);
	TOptional<bool> WasLastInitializationSuccessful{};
	FOnRiveFileInitialized OnInitializedOnceDelegate;
	
	UPROPERTY(Transient, meta=(NoResetToDefault))
	ERiveInitState InitState = ERiveInitState::Uninitialized;

	UPROPERTY()
	TArray<uint8> RiveFileData;

	UPROPERTY(meta=(NoResetToDefault))
	TSubclassOf<UUserWidget> WidgetClass;

public:
	UPROPERTY()
	TArray<URiveArtboard*> Artboards;
	
	UPROPERTY(VisibleAnywhere, Category=Rive, meta=(NoResetToDefault))
	TMap<uint32, TObjectPtr<URiveAsset>> Assets;

	TMap<uint32, TObjectPtr<URiveAsset>>& GetAssets()
	{
		return Assets;
	}
	
	rive::Span<const uint8> RiveNativeFileSpan;
	rive::Span<const uint8>& GetNativeFileSpan()
	{
		return RiveNativeFileSpan;
	}


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

	bool EditorImport(const FString& InRiveFilePath, TArray<uint8>& InRiveFileBuffer, bool bIsReimport = false);

#endif // WITH_EDITOR

	bool bNeedsImport = false;
};
