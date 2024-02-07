// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveEvent.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/custom_property_boolean.hpp"
#include "rive/custom_property_number.hpp"
#include "rive/custom_property_string.hpp"
#include "rive/event.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

const FString& URiveEvent::GetName() const
{
	return Name;
}

uint8 URiveEvent::GetType() const
{
	return Type;
}

float URiveEvent::GetDelayInSeconds() const
{
	return DelayInSeconds;
}

const TArray<FRiveEventBoolProperty>& URiveEvent::GetBoolProperties() const
{
	return RiveEventBoolProperties;
}

const TArray<FRiveEventNumberProperty>& URiveEvent::GetNumberProperties() const
{
	return RiveEventNumberProperties;
}

const TArray<FRiveEventStringProperty>& URiveEvent::GetStringProperties() const
{
	return RiveEventStringProperties;
}

#if WITH_RIVE

void URiveEvent::Initialize(const rive::EventReport& InEventReport)
{
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
			for (size_t PropertyIndex = 0; PropertyIndex < NumProperties; ++PropertyIndex)
			{
				if (rive::Component* Child = NativeEvent->children().at(PropertyIndex))
				{
					if (!Child->is<rive::CustomProperty>())
					{
						continue;
					}

					if (rive::CustomProperty* Property = Child->as<rive::CustomProperty>())
					{
						FString PropertyName = Property->name().c_str();

						switch (Property->coreType())
						{
							case NumberProperty: // Number Property
							{
								if (rive::CustomPropertyNumber* NumProperty = Property->as<rive::CustomPropertyNumber>())
								{
									ParseProperties<float>(TPair<FString, float>(PropertyName, NumProperty->propertyValue()));
								}

								break;
							}
							case BooleanProperty: // Boolean Property
							{
								if (rive::CustomPropertyBoolean* BoolProperty = Property->as<rive::CustomPropertyBoolean>())
								{
									ParseProperties<bool>(TPair<FString, bool>(PropertyName, BoolProperty->propertyValue()));
								}

								break;
							}
							case StringProperty: // String Property
							{
								if (rive::CustomPropertyString* StrProperty = Property->as<rive::CustomPropertyString>())
								{
									ParseProperties<FString>(TPair<FString, FString>(PropertyName, StrProperty->propertyValue().c_str()));
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

#endif // WITH_RIVE
