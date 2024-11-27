#pragma once
#include "RiveTypes.h"
#include "RiveDescriptor.generated.h"

enum class ERiveAlignment : uint8;
enum class ERiveFitType : uint8;
class URiveFile;

/*
 * Describes information about instantiating an artboard from a RiveFile
 */
USTRUCT(BlueprintType)
struct FRiveDescriptor
{
public:
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Rive)
    TObjectPtr<URiveFile> RiveFile;

    // Artboard Name is used if specified, otherwise ArtboardIndex will always
    // be used
    UPROPERTY(BlueprintReadWrite,
              EditAnywhere,
              Category = Rive,
              meta = (GetOptions = "GetArtboardNamesForDropdown"))
    FString ArtboardName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Rive)
    int32 ArtboardIndex = 0;

    // StateMachine name to pass into our default artboard instance
    UPROPERTY(BlueprintReadWrite,
              EditAnywhere,
              Category = Rive,
              meta = (GetOptions = "GetStateMachineNamesForDropdown"))
    FString StateMachineName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Rive)
    ERiveFitType FitType = ERiveFitType::Contain;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Rive)
    ERiveAlignment Alignment = ERiveAlignment::Center;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Rive)
    float ScaleFactor = 1.0f;
};
