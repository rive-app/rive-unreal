// Copyright 2024-2026 Rive, Inc. All rights reserved.

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
    AssetBlob = 14,
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

    bool operator==(const FRivePropertyData& Other) const
    {
        return Name == Other.Name && MetaData == Other.MetaData &&
               Type == Other.Type;
    }
    bool operator!=(const FRivePropertyData& Other) const
    {
        return !(*this == Other);
    }
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
    // The artboard's size in the file, measured at import.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    FVector2D DefaultArtboardSize = FVector2D::ZeroVector;
};

USTRUCT(BlueprintType)
struct FPropertyDefaultData
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    FString Name;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    FString Value;

    bool operator==(const FPropertyDefaultData& Other) const
    {
        return Name == Other.Name && Value == Other.Value;
    }
    bool operator!=(const FPropertyDefaultData& Other) const
    {
        return !(*this == Other);
    }
};

USTRUCT(BlueprintType)
struct FViewModelInstanceDefaultData
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    FString InstanceName;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    TArray<FPropertyDefaultData> PropertyValues;

    bool operator==(const FViewModelInstanceDefaultData& Other) const
    {
        return InstanceName == Other.InstanceName &&
               PropertyValues == Other.PropertyValues;
    }
    bool operator!=(const FViewModelInstanceDefaultData& Other) const
    {
        return !(*this == Other);
    }
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

    bool operator==(const FViewModelDefinition& Other) const
    {
        return Name == Other.Name && InstanceNames == Other.InstanceNames &&
               PropertyDefinitions == Other.PropertyDefinitions &&
               InstanceDefaults == Other.InstanceDefaults &&
               DefaultInstanceName == Other.DefaultInstanceName;
    }
    bool operator!=(const FViewModelDefinition& Other) const
    {
        return !(*this == Other);
    }
};