// Copyright Rive, Inc. All rights reserved.

#pragma once

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/animation/state_machine_instance.hpp"
#include "rive/custom_property.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

namespace UE::Rive::Core
{
	class FUREvent;

	/**
	 * Type definition for unique pointer reference to the instance of FUREvent.
	 */
	using FUREventPtr = TUniquePtr<FUREvent>;

	/**
	 * Represents an event reported by a StateMachine.
	 */
	class RIVE_API FUREvent
	{
		/**
		 * Structor(s)
		 */

	public:

		FUREvent() = default;

		~FUREvent() = default;

#if WITH_RIVE

		explicit FUREvent(const rive::EventReport& InEventReport);

		/**
		 * Implementation(s)
		 */

	public:

		bool GetBoolValue(const FString& InPropertyName) const;

		const FString& GetName() const;

		float GetDelayInSeconds() const;

		float GetNumberValue(const FString& InPropertyName) const;

		const FString GetStringValue(const FString& InPropertyName) const;

		uint8 GetType() const;

		bool HasBoolValue(const FString& InPropertyName) const;

		bool HasNumberValue(const FString& InPropertyName) const;

		bool HasStringValue(const FString& InPropertyName) const;

		/**
		 * Attribute(s)
		 */

	private:

		FString Name = "None";

		float DelayInSeconds = 0.f;

		uint8 Type = 0U;

		TMap<FString, bool> BoolProperties;

		TMap<FString, float> NumberProperties;

		TMap<FString, FString> StringProperties;

#endif // WITH_RIVE
	};
}
