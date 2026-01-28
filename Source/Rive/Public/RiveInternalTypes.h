// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RiveInternalTypes.generated.h"
// Copy of rive::DataType to place nice with unreal.
UENUM(BlueprintType)
enum class ERiveDataType : uint8
{
    None = 0,
    String = 1,
    Number = 2,
    Boolean = 3,
    Color = 4,
    List = 5,
    EnumType = 6,
    Trigger = 7,
    ViewModel = 8,
    AssetImage = 11,
    Artboard = 12,
    SymbolListIndex = 13,
};

USTRUCT(BlueprintType)
struct FRivePropertyData
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    FString Name;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    FString MetaData;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    ERiveDataType Type = ERiveDataType::None;
};

USTRUCT(BlueprintType)
struct FEnumDefinition
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    FString Name;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    TArray<FString> Values;
};

USTRUCT(BlueprintType)
struct FArtboardDefinition
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    FString Name;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    TArray<FString> StateMachineNames;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    FString DefaultViewModel;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    FString DefaultViewModelInstance;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    FVector2D DefaultArtboardSize;
};

USTRUCT(BlueprintType)
struct FPropertyDefaultData
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    FString Name;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    FString Value;
};

USTRUCT(BlueprintType)
struct FViewModelInstanceDefaultData
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    FString InstanceName;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    TArray<FPropertyDefaultData> PropertyValues;
};

USTRUCT(BlueprintType)
struct FViewModelDefinition
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    FString Name;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    TArray<FString> InstanceNames;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    TArray<FRivePropertyData> PropertyDefinitions;
    // Used to store instance default data. This is a work around so that we do
    // not need an event for data is ready. {Instance Name : { Property Name :
    // Property Default Value Encoded As Strings} }
    UPROPERTY()
    TArray<FViewModelInstanceDefaultData> InstanceDefaults;
    // This is the name of the default instance, so we can load its values when
    // the vm is made.
    UPROPERTY()
    FString DefaultInstanceName;

    FORCEINLINE bool operator==(const FViewModelDefinition& Other) const
    {
        return Other.Name == Name;
    }
};