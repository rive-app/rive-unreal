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
    Artboard = 12
};

USTRUCT(BlueprintType)
struct FRivePropertyData
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString Name;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString MetaData;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    ERiveDataType Type = ERiveDataType::None;
};

USTRUCT(BlueprintType)
struct FEnumDefinition
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString Name;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FString> Values;
};

USTRUCT(BlueprintType)
struct FArtboardDefinition
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString Name;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FString> StateMachineNames;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString DefaultViewModel;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString DefaultViewModelInstance;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FVector2D DefaultArtboardSize;
};

USTRUCT(BlueprintType)
struct FViewModelDefinition
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString Name;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FString> InstanceNames;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FRivePropertyData> PropertyDefinitions;
};