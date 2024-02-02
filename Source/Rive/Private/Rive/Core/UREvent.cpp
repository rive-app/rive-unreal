// Copyright Rive, Inc. All rights reserved.
#include "Rive/Core/UREvent.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/custom_property_boolean.hpp"
#include "rive/custom_property_number.hpp"
#include "rive/custom_property_string.hpp"
#include "rive/event.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

#if WITH_RIVE

UE::Rive::Core::FUREvent::FUREvent(const rive::EventReport& InEventReport)
	: DelayInSeconds(InEventReport.secondsDelay())
	, BoolProperties()
	, NumberProperties()
	, StringProperties()
{
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
							case 127: // Number Property
							{
								if (rive::CustomPropertyNumber* NumProperty = Property->as<rive::CustomPropertyNumber>())
								{
									NumberProperties.Add(PropertyName, NumProperty->propertyValue());
								}

								break;
							}
							case 129: // Boolean Property
							{
								if (rive::CustomPropertyBoolean* BoolProperty = Property->as<rive::CustomPropertyBoolean>())
								{
									BoolProperties.Add(PropertyName, BoolProperty->propertyValue());
								}

								break;
							}
							case 130: // String Property
							{
								if (rive::CustomPropertyString* StrProperty = Property->as<rive::CustomPropertyString>())
								{
									StringProperties.Add(PropertyName, StrProperty->propertyValue().c_str());
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

bool UE::Rive::Core::FUREvent::GetBoolValue(const FString& InPropertyName) const
{
	if (HasBoolValue(InPropertyName))
	{
		return BoolProperties[InPropertyName];
	}

	return false;
}

const FString& UE::Rive::Core::FUREvent::GetName() const
{
	return Name;
}

float UE::Rive::Core::FUREvent::GetDelayInSeconds() const
{
	return DelayInSeconds;
}

float UE::Rive::Core::FUREvent::GetNumberValue(const FString& InPropertyName) const
{
	if (HasNumberValue(InPropertyName))
	{
		return NumberProperties[InPropertyName];
	}

	return 0.f;
}

const FString UE::Rive::Core::FUREvent::GetStringValue(const FString& InPropertyName) const
{
	if (HasStringValue(InPropertyName))
	{
		return StringProperties[InPropertyName];
	}

	return "None";
}

uint8 UE::Rive::Core::FUREvent::GetType() const
{
	return Type;
}

bool UE::Rive::Core::FUREvent::HasBoolValue(const FString& InPropertyName) const
{
	return BoolProperties.Contains(InPropertyName);
}

bool UE::Rive::Core::FUREvent::HasNumberValue(const FString& InPropertyName) const
{
	return NumberProperties.Contains(InPropertyName);
}

bool UE::Rive::Core::FUREvent::HasStringValue(const FString& InPropertyName) const
{
	return StringProperties.Contains(InPropertyName);
}

#endif // WITH_RIVE
