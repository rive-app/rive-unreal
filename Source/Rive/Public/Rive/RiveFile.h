// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRenderTarget.h"
#include "RiveArtboard.h"
#include "RiveEvent.h"
#include "RiveTexture.h"
#include "RiveTypes.h"
#include "Tickable.h"
#include "RiveFile.generated.h"

#if WITH_RIVE

struct FAssetImportInfo;
class URiveArtboard;
class FRiveTextureResource;

namespace rive
{
	class File;
}

#endif // WITH_RIVE

class URiveAsset;
class UUserWidget;

/**
 *
 */
UCLASS(BlueprintType, Blueprintable, HideCategories="ImportSettings")
class RIVE_API URiveFile : public URiveTexture, public FTickableGameObject
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnArtboardChangedDynamic, URiveFile*, RiveFile, URiveArtboard*, Artboard);
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnArtboardChanged, URiveFile* /* RiveFile */, URiveArtboard* /* Artboard */);
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRiveFileInitialized, URiveFile*, bool /* bSuccess */ );
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnRiveFileEvent, URiveFile*);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRiveReadyDelegate);
	
	/**
	 * Structor(s)
	 */
	
	URiveFile();
	
	virtual void BeginDestroy() override;
	
	//~ BEGIN : FTickableGameObject Interface

public:
	virtual TStatId GetStatId() const override;

	virtual void Tick(float InDeltaSeconds) override;

	virtual bool IsTickable() const override;

	virtual bool IsTickableInEditor() const override
	{
		return true;
	}

	virtual ETickableTickType GetTickableTickType() const override
	{
		return ETickableTickType::Conditional;
	}

	//~ END : FTickableGameObject Interface

	//~ BEGIN : UObject Interface
	virtual void PostLoad() override;

#if WITH_EDITOR

	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;

#endif // WITH_EDITOR

	//~ END : UObject Interface

	/**
	 * Implementation(s)
	 */

public:
	// Called to create a new rive file instance at runtime
	UFUNCTION(BlueprintCallable, Category = Rive)
	URiveFile* CreateInstance(const FString& InArtboardName, const FString& InStateMachineName);

	UFUNCTION(BlueprintCallable, Category = Rive, meta=(DeprecatedFunction, DeprecationMessage="Use RiveFile->Artboard->FireTrigger instead"))
	void FireTrigger(const FString& InPropertyName) const;

	UFUNCTION(BlueprintCallable, Category = Rive, meta=(DeprecatedFunction, DeprecationMessage="Use RiveFile->Artboard->GetBoolValue instead"))
	bool GetBoolValue(const FString& InPropertyName) const;

	UFUNCTION(BlueprintCallable, Category = Rive, meta=(DeprecatedFunction, DeprecationMessage="Use RiveFile->Artboard->GetNumberValue instead"))
	float GetNumberValue(const FString& InPropertyName) const;

	UFUNCTION(BlueprintPure, Category = Rive)
	FLinearColor GetClearColor() const;

	UFUNCTION(BlueprintCallable, Category = Rive)
	FVector2f GetLocalCoordinate(URiveArtboard* InArtboard, const FVector2f& InPosition);

	/**
	 * Returns the coordinates in the current Artboard space
	 * @param InExtents Extents of the RenderTarget, will be mapped to the RenderTarget size
	 */
	UFUNCTION(BlueprintCallable, Category = Rive)
	FVector2f GetLocalCoordinatesFromExtents(const FVector2f& InPosition, const FBox2f& InExtents) const;
	
	UFUNCTION(BlueprintCallable, Category = Rive, meta=(DeprecatedFunction, DeprecationMessage="Use RiveFile->Artboard->SetBoolValue instead"))
	void SetBoolValue(const FString& InPropertyName, bool bNewValue);

	UFUNCTION(BlueprintCallable, Category = Rive, meta=(DeprecatedFunction, DeprecationMessage="Use RiveFile->Artboard->SetNumberValue instead"))
	void SetNumberValue(const FString& InPropertyName, float NewValue);

#if WITH_EDITOR

	bool EditorImport(const FString& InRiveFilePath, TArray<uint8>& InRiveFileBuffer, bool bIsReimport = false);

#endif // WITH_EDITOR

	/**
	 * Initialize this Rive file by creating the Render Targets and importing the native Rive File 
	 */
	void Initialize();
	void InstantiateArtboard(bool bRaiseArtboardChangedEvent = true);
	void SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass);

	TSubclassOf<UUserWidget> GetWidgetClass() const { return WidgetClass; }

	UFUNCTION(BlueprintCallable, Category = Rive)
	URiveArtboard* GetArtboard() const;

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
private:
	void BroadcastInitializationResult(bool bSuccess);
	TOptional<bool> WasLastInitializationSuccessful{};
	FOnRiveFileInitialized OnInitializedOnceDelegate;
	
protected:

	void OnResourceInitialized_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRHIRef& NewResource) const;

public:
	UPROPERTY(BlueprintAssignable, Category = Rive)
	FOnArtboardChangedDynamic OnArtboardChanged;
	FOnArtboardChanged OnArtboardChangedRaw;

	UPROPERTY()
	TArray<uint8> RiveFileData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rive, meta=(NoResetToDefault))
	FString RiveFilePath;

	UPROPERTY(VisibleAnywhere, Category=Rive)
	TMap<uint32, TObjectPtr<URiveAsset>> Assets;

	TMap<uint32, TObjectPtr<URiveAsset>>& GetAssets()
	{
		return IsValid(ParentRiveFile) ? ParentRiveFile->GetAssets() : Assets;
	}

	rive::File* GetNativeFile() const
	{
		if (IsValid(ParentRiveFile))
		{
			return ParentRiveFile->GetNativeFile();
		}
		else if (RiveNativeFilePtr)
		{
			return RiveNativeFilePtr.get();
		}

		return nullptr;
	}

	UPROPERTY(VisibleAnywhere, Category=Rive)
	TObjectPtr<URiveFile> ParentRiveFile;

public:
	// Index of the artboard this Rive file instance will default to; not exposed
	UPROPERTY(BlueprintReadWrite, Category=Rive)
	int32 ArtboardIndex;

	// Artboard Name is used if specified, otherwise ArtboardIndex will always be used
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Rive, meta=(GetOptions="GetArtboardNamesForDropdown"))
	FString ArtboardName;

	// StateMachine name to pass into our default artboard instance
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Rive, meta=(GetOptions="GetStateMachineNamesForDropdown"))
	FString StateMachineName;

private:
	UFUNCTION()
	void OnArtboardTickRender(float DeltaTime, URiveArtboard* InArtboard);
	
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
		return Artboard ? Artboard->GetStateMachineNamesForDropdown() : TArray<FString>{};
	}
	
	UPROPERTY(EditAnywhere, Category = Rive)
	FLinearColor ClearColor = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, Category = Rive)
	ERiveFitType RiveFitType = ERiveFitType::Contain;

	/* This property is not editable via Editor in Unity, so we'll hide it also */
	UPROPERTY()
	ERiveAlignment RiveAlignment = ERiveAlignment::Center;

	UPROPERTY(EditAnywhere, Category = Rive)
	bool bIsRendering = true;

	/** Control Size of Render Texture Manually */
	UPROPERTY(EditAnywhere, Category = Rive)
	bool bManualSize = false;

	UPROPERTY(EditAnywhere, Category=Rive)
	TSubclassOf<UUserWidget> WidgetClass;

	UPROPERTY(VisibleInstanceOnly, Transient, Category=Rive, meta=(NoResetToDefault))
	ERiveInitState InitState = ERiveInitState::Uninitialized;

	UE::Rive::Renderer::IRiveRenderTargetPtr RiveRenderTarget;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category=Rive, meta=(NoResetToDefault, AllowPrivateAccess, ShowInnerProperties))
	URiveArtboard* Artboard = nullptr;

	rive::Span<const uint8> RiveNativeFileSpan;

	rive::Span<const uint8>& GetNativeFileSpan()
	{
		if (ParentRiveFile)
		{
			return ParentRiveFile->GetNativeFileSpan();
		}

		return RiveNativeFileSpan;
	}


	std::unique_ptr<rive::File> RiveNativeFilePtr;
	
	void PrintStats() const;


#if WITH_EDITORONLY_DATA
	void OnImportDataChanged(const FAssetImportInfo& OldData, const class UAssetImportData* NewData);
#endif

	bool bNeedsImport = false;
};
