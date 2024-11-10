// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveEvent.h"

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "Logs/RiveLog.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/custom_property_boolean.hpp"
#include "rive/custom_property_number.hpp"
#include "rive/custom_property_string.hpp"
#include "rive/event.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

#if WITH_RIVE

void FRiveEvent::Initialize(const rive::EventReport& InEventReport)
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to Initialize the RiveEvent as we do not have a "
                    "valid renderer."));
        return;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    DelayInSeconds = InEventReport.secondsDelay();

    RiveEventBoolProperties.Reset();

    RiveEventNumberProperties.Reset();

    RiveEventStringProperties.Reset();

    if (rive::Event* NativeEvent = InEventReport.event())
    {
        Name = NativeEvent->name().c_str();

        Type = NativeEvent->coreType();

        const size_t NumProperties = NativeEvent->children().size();

        if (NumProperties != 0)
        {
            for (size_t PropertyIndex = 0; PropertyIndex < NumProperties;
                 ++PropertyIndex)
            {
                if (rive::Component* Child =
                        NativeEvent->children().at(PropertyIndex))
                {
                    if (!Child->is<rive::CustomProperty>())
                    {
                        continue;
                    }

                    if (rive::CustomProperty* Property =
                            Child->as<rive::CustomProperty>())
                    {
                        FString PropertyName = Property->name().c_str();

                        switch (Property->coreType())
                        {
                            case NumberProperty: // Number Property
                            {
                                if (rive::CustomPropertyNumber* NumProperty =
                                        Property
                                            ->as<rive::CustomPropertyNumber>())
                                {
                                    ParseProperties<float>(
                                        TPair<FString, float>(
                                            PropertyName,
                                            NumProperty->propertyValue()));
                                }

                                break;
                            }
                            case BooleanProperty: // Boolean Property
                            {
                                if (rive::CustomPropertyBoolean* BoolProperty =
                                        Property
                                            ->as<rive::CustomPropertyBoolean>())
                                {
                                    ParseProperties<bool>(TPair<FString, bool>(
                                        PropertyName,
                                        BoolProperty->propertyValue()));
                                }

                                break;
                            }
                            case StringProperty: // String Property
                            {
                                if (rive::CustomPropertyString* StrProperty =
                                        Property
                                            ->as<rive::CustomPropertyString>())
                                {
                                    ParseProperties<FString>(TPair<FString,
                                                                   FString>(
                                        PropertyName,
                                        StrProperty->propertyValue().c_str()));
                                }

                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

bool FRiveEvent::operator==(const FRiveEvent& InRiveFile) const
{
    return Id == InRiveFile.Id;
}

bool FRiveEvent::operator==(FGuid InEntityId) const { return Id == InEntityId; }

uint32 GetTypeHash(const FRiveEvent& InRiveFile)
{
    return GetTypeHash(InRiveFile.Id);
}

#endif // WITH_RIVE
