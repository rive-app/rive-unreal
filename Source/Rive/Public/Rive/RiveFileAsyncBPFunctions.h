// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RiveFile.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "UObject/Object.h"
#include "RiveFileAsyncBPFunctions.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRiveFileAsyncEvent);

/**
 * Class to call the Async UGFPakPlugin::ActivateGameFeature from Blueprints
 */
UCLASS()
class RIVE_API URiveFileWhenReadyAsync : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	 * Runs the Ready branch when the RiveFile is Initialized. If it is already Initialized, it will run it right away
	 */
	UFUNCTION(BlueprintCallable, DisplayName="When Ready", Category="River", meta = (BlueprintInternalUseOnly = "true"))
	static URiveFileWhenReadyAsync* RiveFileWhenReadyAsync(URiveFile* RiveFile, UPARAM(DisplayName = "Rive File") URiveFile*& OutRiveFile);

	/**
	 * Called when the RiveFile has successfully initialized.
	 * In case the RiveFile is already initialized, this would be run first before the Then pin.
	 */
	UPROPERTY(BlueprintAssignable)
	FRiveFileAsyncEvent Ready;
	
	/**
	 * Called when the RiveFile initialization failed
	 * In case the RiveFile initialization has already failed, this would be run first before the Then pin.
	 */
	UPROPERTY(BlueprintAssignable)
	FRiveFileAsyncEvent OnInitializedFailed;
	
	// Start UBlueprintAsyncActionBase Functions
	virtual void Activate() override;
	// End UBlueprintAsyncActionBase Functions
private:
	UPROPERTY()
	URiveFile* RiveFile = nullptr;

	void OnRiveFileInitialized(URiveFile* RiveFile, bool bSuccess);

	void ReportActivated();
	void ReportFailed();
};
