#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "RiveViewModelPropertyInterface.generated.h"

UINTERFACE(Blueprintable)
class RIVE_API URiveViewModelPropertyInterface : public UInterface
{
    GENERATED_BODY()
};

class RIVE_API IRiveViewModelPropertyInterface
{
    GENERATED_BODY()

public:
    static FString GetSuffix() { return FString(TEXT("")); }
};