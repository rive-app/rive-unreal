// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "KismetCompiler.h"
#include "UObject/Object.h"
#include "PinTools.generated.h"

/**
 *
 */
UCLASS()
class RIVEEDITORNODES_API UPinTools : public UObject
{
    GENERATED_BODY()

public:
    FORCEINLINE static void MovePinLinksOrCopyDefaults(
        FKismetCompilerContext& CompilerContext,
        UEdGraphPin* Source,
        UEdGraphPin* Dest)
    {
        if (Source->LinkedTo.Num() > 0)
        {
            CompilerContext.MovePinLinksToIntermediate(*Source, *Dest);
        }
        else
        {
            Dest->DefaultObject = Source->DefaultObject;
            Dest->DefaultValue = Source->DefaultValue;
            Dest->DefaultTextValue = Source->DefaultTextValue;
        }
    }

    FORCEINLINE static FString GetPinStringValue(UEdGraphPin* Pin)
    {
        check(Pin);
        if (Pin->HasAnyConnections())
        {
            for (auto ConnectedPin : Pin->LinkedTo)
            {
                if (ConnectedPin->DefaultObject)
                    return ConnectedPin->GetDefaultAsString();
            }
        }

        return Pin->GetDefaultAsString();
    }

    template <typename ReturnType>
    static TObjectPtr<ReturnType> GetPinObjectValue(UEdGraphPin* Pin)
    {
        check(Pin);
        if (Pin->HasAnyConnections())
        {
            for (auto ConnectedPin : Pin->LinkedTo)
            {
                if (ConnectedPin->DefaultObject)
                    return Cast<ReturnType>(ConnectedPin->DefaultObject);
            }
        }

        return Cast<ReturnType>(Pin->DefaultObject);
    }
};
