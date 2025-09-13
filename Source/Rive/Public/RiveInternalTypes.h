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
struct FViewModelDefinition
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    FString Name;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    TArray<FString> InstanceNames;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RiveFileData")
    TArray<FRivePropertyData> PropertyDefinitions;
};