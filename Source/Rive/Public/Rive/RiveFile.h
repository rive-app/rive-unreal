// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRenderTarget.h"
#include "RiveArtboard.h"
#include "RiveTypes.h"
#include "RiveTexture.h"
#include "RiveEvent.h"
#include "RiveFile.generated.h"

#if WITH_RIVE

class URiveArtboard;
class FRiveTextureResource;

namespace rive
{
	class File;
}

#endif // WITH_RIVE

class URiveAsset;

/**
 *
 */
UCLASS(BlueprintType, Blueprintable)
class RIVE_API URiveFile : public URiveTexture, public FTickableGameObject
{
	GENERATED_BODY()
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnArtboardChanged, URiveFile*, RiveFile, URiveArtboard*, Artboard);

	/**
	 * Structor(s)
	 */

public:
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

	UFUNCTION(BlueprintCallable, Category = Rive)
	void FireTrigger(const FString& InPropertyName) const;

	UFUNCTION(BlueprintCallable, Category = Rive)
	bool GetBoolValue(const FString& InPropertyName) const;

	UFUNCTION(BlueprintCallable, Category = Rive)
	float GetNumberValue(const FString& InPropertyName) const;

	UFUNCTION(BlueprintPure, Category = Rive)
	FLinearColor GetClearColor() const;

	UFUNCTION(BlueprintCallable, Category = Rive)
	FVector2f GetLocalCoordinates(const FVector2f& InTexturePosition);

	UFUNCTION(BlueprintCallable, Category = Rive)
	void SetBoolValue(const FString& InPropertyName, bool bNewValue);

	UFUNCTION(BlueprintCallable, Category = Rive)
	void SetNumberValue(const FString& InPropertyName, float NewValue);

	ESimpleElementBlendMode GetSimpleElementBlendMode() const;

#if WITH_EDITOR

	bool EditorImport(const FString& InRiveFilePath, TArray<uint8>& InRiveFileBuffer);

#endif // WITH_EDITOR

	/**
	 * Initialize this Rive file by creating the Render Targets and importing the native Rive File 
	 */
	void Initialize();

	void SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass);

	TSubclassOf<UUserWidget> GetWidgetClass() const { return WidgetClass; }

	URiveArtboard* GetArtboard() const;

	ERiveInitState InitializationState() const { return InitState; }
	UFUNCTION(BlueprintPure, Category = Rive)
	bool IsInitialized() const { return InitState == ERiveInitState::Initialized; }
	
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRiveFileInitialized, URiveFile*, bool /* bSuccess */ );
	virtual void CallOrRegister_OnInitialized(FOnRiveFileInitialized::FDelegate&& Delegate);
private:
	void BroadcastInitializationResult(bool bSuccess);
	TOptional<bool> WasLastInitializationSuccessful{};
	FOnRiveFileInitialized OnInitializedDelegate;
	
protected:
	void InstantiateArtboard(bool bRaiseArtboardChangedEvent = true);

	virtual void OnResourceInitialized_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRHIRef& NewResource) const override;

public:
	UPROPERTY(BlueprintAssignable, Category = Rive)
	FOnArtboardChanged OnArtboardChanged;

	UPROPERTY()
	TArray<uint8> RiveFileData;

	UPROPERTY()
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
	ERiveBlendMode RiveBlendMode = ERiveBlendMode::SE_BLEND_Opaque;

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
};
