// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRenderTarget.h"
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

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRiveStateMachineDelegate, FRiveStateMachineEvent,
												RiveStateMachineEvent);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRiveEventDelegate, int32, NumEvents);

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
	FLinearColor GetDebugColor() const;

	UFUNCTION(BlueprintCallable, Category = Rive)
	FVector2f GetLocalCoordinates(const FVector2f& InTexturePosition) const;

	/**
	 * Returns the coordinates in the current Artboard space
	 * @param InExtents Extents of the RenderTarget, will be mapped to the RenderTarget size
	 */
	UFUNCTION(BlueprintCallable, Category = Rive)
	FVector2f GetLocalCoordinatesFromExtents(const FVector2f& InPosition, const FBox2f& InExtents) const;

	UFUNCTION(BlueprintCallable, Category = Rive)
	void SetBoolValue(const FString& InPropertyName, bool bNewValue);

	UFUNCTION(BlueprintCallable, Category = Rive)
	void SetNumberValue(const FString& InPropertyName, float NewValue);

	FVector2f GetRiveAlignment() const;

	ESimpleElementBlendMode GetSimpleElementBlendMode() const;

	void BeginInput()
	{
		bIsReceivingInput = true;
	}

	void EndInput()
	{
		bIsReceivingInput = false;
	}

#if WITH_EDITOR

	bool EditorImport(const FString& InRiveFilePath, TArray<uint8>& InRiveFileBuffer);

#endif // WITH_EDITOR

	/**
	 * Initialize this Rive file by creating the Render Targets and importing the native Rive File 
	 */
	void Initialize();

	void SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass);

	TSubclassOf<UUserWidget> GetWidgetClass() const { return WidgetClass; }

	const URiveArtboard* GetArtboard() const;

protected:
	void InstantiateArtboard();
	
private:
	void PopulateReportedEvents();
	
	URiveArtboard* InstantiateArtboard_Internal();

public:
	// This Event is triggered any time new LiveLink data is available, including in the editor
	UPROPERTY(BlueprintAssignable, Category = "LiveLink")
	FRiveStateMachineDelegate OnRiveStateMachineDelegate;

	UPROPERTY()
	TArray<uint8> RiveFileData;

	UPROPERTY()
	FString RiveFilePath;

	// TODO. REMOVE IT!!, just for testing
	UPROPERTY(EditAnywhere, Category = Rive)
	bool bUseViewportClientTestProperty = true;

	UPROPERTY(VisibleAnywhere, Category=Rive)
	TMap<uint32, TObjectPtr<URiveAsset>> Assets;

	TMap<uint32, TObjectPtr<URiveAsset>>& GetAssets()
	{
		if (ParentRiveFile)
		{
			return ParentRiveFile->GetAssets();
		}

		return Assets;
	}

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<URiveFile> ParentRiveFile;

protected:
	UPROPERTY(BlueprintAssignable)
	FRiveEventDelegate RiveEventDelegate;

	UPROPERTY(BlueprintReadWrite, Category = Rive)
	TArray<FRiveEvent> TickRiveReportedEvents;

public:
	// Index of the artboard this Rive file instance will default to; not exposed
	UPROPERTY(BlueprintReadWrite, Category=Rive)
	int32 ArtboardIndex;

	// Artboard Name is used if specified, otherwise ArtboardIndex will always be used
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Rive)
	FString ArtboardName;

	// StateMachine name to pass into our default artboard instance
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Rive)
	FString StateMachineName;

private:
	// TODO: DebugColor now unused
	UPROPERTY(EditAnywhere, Category = Rive)
	FLinearColor DebugColor = FLinearColor::Transparent;

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

	bool bIsFileImported = false; //todo: find a better way to do this
	bool bIsInitialized = false;

	bool bIsReceivingInput = false;

	UE::Rive::Renderer::IRiveRenderTargetPtr RiveRenderTarget;

	UPROPERTY(Transient)
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
	rive::File* GetNativeFile() const
	{
		if (ParentRiveFile)
		{
			return ParentRiveFile->GetNativeFile();
		}

		if (RiveNativeFilePtr)
		{
			return RiveNativeFilePtr.get();
		}

		return nullptr;
	}
	
	void PrintStats() const;
};
