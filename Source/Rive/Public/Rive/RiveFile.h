// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include <memory>
#include "Assets/RiveAsset.h"
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "rive/command_queue.hpp"
#include "UObject/Object.h"

#if WITH_RIVE

enum class ETestEnum : uint8;
THIRD_PARTY_INCLUDES_START
#include "rive/command_queue.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

#include "RiveInternalTypes.h"
#include "RiveFile.generated.h"

class URiveAsset;
class URiveArtboard;
class URiveViewModel;
class UAssetImportData;
struct FRiveCommandBuilder;

static FName GViewModelInstanceBlankName = "--Blank--";
static FName GViewModelInstanceDefaultName = "--Default--";

USTRUCT()
struct FGeneratedClassEntry
{
    GENERATED_BODY()
    UPROPERTY()
    TSoftClassPtr<URiveViewModel> Entry;
};

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
    // This means all the data needed to display this file has been gathered
    DECLARE_MULTICAST_DELEGATE_OneParam(FDataReadyDelegate,
                                        TObjectPtr<URiveFile>);

    virtual void BeginDestroy() override;
    virtual void PostLoad() override;
    void Initialize();
    void Initialize(FRiveCommandBuilder&);

    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true"),
              Category = Rive)
    static URiveArtboard* MakeArtboard(URiveFile* InRiveFile,
                                       const FString& Name,
                                       bool inAutoBindViewModel)
    {
        return InRiveFile->CreateArtboardNamed(Name, inAutoBindViewModel);
    }

    UFUNCTION(BlueprintCallable, Category = Rive)
    URiveArtboard* CreateArtboardNamed(const FString& Name,
                                       bool inAutoBindViewModel);

    URiveArtboard* CreateArtboardNamed(FRiveCommandBuilder&,
                                       const FString& Name,
                                       bool inAutoBindViewModel);

    const FViewModelDefinition* GetViewModelDefinition(
        const FString& ViewModelName)
    {
        for (const auto& ViewModelDefinition : ViewModelDefinitions)
        {
            if (ViewModelDefinition.Name == ViewModelName)
            {
                return &ViewModelDefinition;
            }
        }
        return nullptr;
    }

    const FArtboardDefinition* GetArtboardDefinition(const FString& Name)
    {
        if (Name.IsEmpty() && ArtboardDefinitions.Num() > 0)
        {
            return &ArtboardDefinitions[0];
        }

        for (const auto& ArtboardDefinition : ArtboardDefinitions)
        {
            if (ArtboardDefinition.Name == Name)
            {
                return &ArtboardDefinition;
            }
        }
        return nullptr;
    }

    FDataReadyDelegate OnDataReady;
#if WITH_EDITOR
    bool GetHasData() const
    {
        return bHasArtboardData && bHasEnumsData && bHasViewModelData;
    }
#else
    // We should always have data on packaged builds
    bool GetHasData() const { return true; }
#endif
    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Rive)
    TArray<FArtboardDefinition> ArtboardDefinitions;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Rive)
    TArray<FEnumDefinition> EnumDefinitions;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Rive)
    TArray<FViewModelDefinition> ViewModelDefinitions;

#if WITH_EDITORONLY_DATA
    // This property holds the import data
    UPROPERTY(VisibleAnywhere, Instanced, Category = "Import Settings")
    TObjectPtr<UAssetImportData> AssetImportData;
#endif

    rive::FileHandle GetNativeFileHandle() const { return NativeFileHandle; }

private:
    UPROPERTY()
    TArray<uint8> RiveFileData;

    UPROPERTY(VisibleAnywhere, Category = "Rive|ViewModels")
    TMap<FName, FGeneratedClassEntry> GeneratedClassMap;

    rive::FileHandle NativeFileHandle = RIVE_NULL_HANDLE;

#if WITH_EDITORONLY_DATA
    // Used for getting default data. Does not provide reflection data.
    void EnumsListed(std::vector<rive::ViewModelEnum> InEnumNames);
    void ArtboardsListed(std::vector<std::string> InArtboardNames);
    void ViewModelNamesListed(std::vector<std::string> InViewModelNames);
    void ViewModelInstanceNamesListed(std::string viewModelName,
                                      std::vector<std::string> InInstanceNames);
    void ViewModelPropertyDefinitionsListed(
        std::string viewModelName,
        std::vector<rive::CommandQueue::FileListener::ViewModelPropertyData>
            properties);
    bool bHasArtboardData = false;
    bool bHasViewModelData = false;
    bool bHasEnumsData = false;
    bool bHasViewModelInstanceDefaultsData = false;
    void CheckShouldBroadcastDataReady();
#endif
    friend class FRiveFileListener;

public:
    // These are used for blueprint node dropdowns. They are generated on file
    // import.

    // Enum of all view model type names in the file.
    UPROPERTY()
    TObjectPtr<UEnum> ViewModelEnum;
    // Map of view model type name to instance names enum
    UPROPERTY()
    TMap<FString, TObjectPtr<UEnum>> ViewModelInstanceEnums;
    // Enum of artboard names in the file.
    UPROPERTY()
    TObjectPtr<UEnum> ArtboardEnum;

    int32 GetViewModelCount() const { return ViewModelDefinitions.Num(); }

    UClass* GetGeneratedClassForViewModel(const FName& ViewModelName) const;
    // Creates a ViewModel Given the ViewModel name and Instance name
    UFUNCTION(BlueprintCallable, Category = "Rive|File")
    static URiveViewModel* CreateViewModelByName(const URiveFile* InputFile,
                                                 const FString& ViewModelName,
                                                 const FString& InstanceName);

    // Creates a default view model for given artboard and isntance name
    UFUNCTION(BlueprintCallable, Category = "Rive|File")
    static URiveViewModel* CreateViewModelByArtboardName(
        const URiveFile* InputFile,
        const FString& ArtboardName,
        const FString& InstanceName);

    // Creates a Default ViewModel Given the ViewModel Name
    UFUNCTION(BlueprintCallable, Category = "Rive|File")
    static URiveViewModel* CreateDefaultViewModel(const URiveFile* InputFile,
                                                  const FString& ViewModelName);

    // Creates the default ViewModel for a given Artboard
    UFUNCTION(BlueprintCallable, Category = "Rive|File")
    static URiveViewModel* CreateDefaultViewModelForArtboard(
        const URiveFile* InputFile,
        URiveArtboard* Artboard);

    // Creates the default ViewModel for a given Artboard and uses the given
    // InstanceName for the chosen instance
    UFUNCTION(BlueprintCallable, Category = "Rive|File")
    static URiveViewModel* CreateArtboardViewModelByName(
        const URiveFile* InputFile,
        URiveArtboard* Artboard,
        const FString& InstanceName);

    std::vector<uint8_t> RiveNativeFileSpan;

    UEnum* GetViewModelInstanceEnum(
        const FViewModelDefinition& ViewModelDefinition) const
    {
        check(ViewModelDefinitions.Find(ViewModelDefinition) != INDEX_NONE);
        check(ViewModelInstanceEnums.Find(ViewModelDefinition.Name) != nullptr);
        return *ViewModelInstanceEnums.Find(ViewModelDefinition.Name);
    }

#if WITH_EDITORONLY_DATA
    // Used to register blueprints classes that are generated for view models.
    void RegisterGeneratedBlueprint(const FName& BlueprintName,
                                    UBlueprintGeneratedClass* GeneratedClass);
    void GenerateViewModelEnum();
    void GenerateViewModelInstanceEnums(
        const FViewModelDefinition& ViewModelDefinition);
    void GenerateArtboardEnum();

#endif

#if WITH_EDITOR
    void PrintStats() const;

    bool EditorImport(const FString& InRiveFilePath,
                      TArray<uint8>& InRiveFileBuffer,
                      bool bIsReimport = false);

#endif // WITH_EDITOR

    bool bNeedsImport = false;
};
