// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RiveEvent.generated.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/animation/state_machine_instance.hpp"
#include "rive/custom_property.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

USTRUCT(Blueprintable)
struct FRiveEventProperty
{
    GENERATED_BODY()

    FRiveEventProperty() = default;

    explicit FRiveEventProperty(const FString& InName)
        : PropertyName(InName)
    {
    }

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rive | Events")
    FString PropertyName = "None";
};

USTRUCT()
struct FRiveEventBoolProperty : public FRiveEventProperty
{
    GENERATED_BODY()

    FRiveEventBoolProperty() = default;

    explicit FRiveEventBoolProperty(const FString& InName, bool InProperty)
        : FRiveEventProperty(InName)
        , BoolProperty(InProperty)
    {
    }

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rive | Events")
    bool BoolProperty = false;
};

USTRUCT()
struct FRiveEventNumberProperty : public FRiveEventProperty
{
    GENERATED_BODY()

    FRiveEventNumberProperty() = default;

    explicit FRiveEventNumberProperty(const FString& InName, float InProperty)
        : FRiveEventProperty(InName)
        , NumberProperty(InProperty)
    {
    }

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rive | Events")
    float NumberProperty = 0.f;
};

USTRUCT(Blueprintable)
struct FRiveEventStringProperty : public FRiveEventProperty
{
    GENERATED_BODY()

    FRiveEventStringProperty() = default;

    FRiveEventStringProperty(const FString& InName, const FString& InProperty)
        : FRiveEventProperty(InName)
        , StringProperty(InProperty)
    {
    }

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rive | Events")
    FString StringProperty = "None";
};

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, Meta = (DisplayName = "Rive Event"))
class RIVE_API URiveEvent : public UObject
{
	GENERATED_BODY()

    /**
     * Structor(s)
     */

public:
    /**
     * Implementation(s)
     */

public:

    UFUNCTION(BlueprintPure, Category = "Rive | Events")
    const FString& GetName() const;

    UFUNCTION(BlueprintPure, Category = "Rive | Events")
    uint8 GetType() const;

    UFUNCTION(BlueprintPure, Category = "Rive | Events")
    float GetDelayInSeconds() const;

    UFUNCTION(BlueprintPure, Category = "Rive | Events")
    const TArray<FRiveEventBoolProperty>& GetBoolProperties() const;

    UFUNCTION(BlueprintPure, Category = "Rive | Events")
    const TArray<FRiveEventNumberProperty>& GetNumberProperties() const;

    UFUNCTION(BlueprintPure, Category = "Rive | Events")
    const TArray<FRiveEventStringProperty>& GetStringProperties() const;

#if WITH_RIVE

    void Initialize(const rive::EventReport& InEventReport);

#endif // WITH_RIVE

private:

    template <typename TPropertyType>
    void ParseProperties(const TPair<FString, TPropertyType>& InPropertyPair);

    /**
     * Attribute(s)
     */

private:

    float DelayInSeconds = 0.f;

    FString Name = "None";

    uint8 Type = 0;

    TArray<FRiveEventBoolProperty> RiveEventBoolProperties;

    TArray<FRiveEventNumberProperty> RiveEventNumberProperties;

    TArray<FRiveEventStringProperty> RiveEventStringProperties;

private:
    static constexpr int32 NumberProperty   = 127;
    static constexpr int32 BooleanProperty  = 129;
    static constexpr int32 StringProperty   = 130;
};

#define PARSE_PROPERTIES(Type, TPropertyType, InPropertyPair) \
FRiveEvent##Type##Property NewRiveEvent(InPropertyPair.Key, InPropertyPair.Value); \
RiveEvent##Type##Properties.Add(NewRiveEvent); \

template <typename TPropertyType>
void URiveEvent::ParseProperties(const TPair<FString, TPropertyType>& InPropertyPair)
{
    if constexpr (std::is_same_v<TPropertyType, bool>)
    {
        PARSE_PROPERTIES(Bool, bool, InPropertyPair);
    }
    else if constexpr (std::is_same_v<TPropertyType, float>)
    {
        PARSE_PROPERTIES(Number, TPropertyType, InPropertyPair);
    }
    else if constexpr (std::is_same_v<TPropertyType, FString>)
    {
        PARSE_PROPERTIES(String, TPropertyType, InPropertyPair);
    }
}

#undef PARSE_PROPERTIES
