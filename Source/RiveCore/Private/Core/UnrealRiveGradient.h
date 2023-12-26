// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Containers/StaticArray.h"
#include "Core/UnrealRiveCoreTypes.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/shapes/paint/color.hpp"
#include "rive/renderer.hpp"
#include "utils/lite_rtti.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

namespace UE::Rive::Core::Private
{
    class FUnrealRiveGradient;

    /**
     * Type definition for unique pointer reference to the instance of FUnrealRiveGradient.
     */
    using FUnrealRiveGradientPtr = std::unique_ptr<FUnrealRiveGradient>;

    /**
     * Copies an array of colors or stops for a gradient.
     * Stores the data locally if there are 4 values or fewer.
     * Spills onto the heap if there are > 4 values.
     */
    template <typename T>
    struct FUnrealRiveGradData
    {
        /**
         * Structor(s)
         */

    public:

        FUnrealRiveGradData()
            : RawData(nullptr)
            , UnrealData()
        {
        }

        explicit FUnrealRiveGradData(const T InRawData[], size_t InSize)
        {
            RawData = InSize <= UnrealData.Num() ? UnrealData.GetData() : new T[InSize];

            FMemory::Memcpy(RawData, InRawData, InSize * sizeof(T));
        }

        explicit FUnrealRiveGradData(FUnrealRiveGradData&& InOther)
        {
            if (InOther.RawData == InOther.UnrealData.GetData())
            {
                UnrealData = InOther.UnrealData;

                RawData = UnrealData.GetData();
            }
            else
            {
                RawData = InOther.RawData;

                InOther.RawData = InOther.UnrealData.GetData(); // Don't delete[] InOther.RawData.
            }
        }

        ~FUnrealRiveGradData()
        {
            if (RawData != UnrealData.GetData())
            {
                delete[] RawData;
            }
        }

        /**
         * Implementation(s)
         */

    public:

        const T* Get() const
        {
            return RawData;
        }

        const T operator[](size_t Index) const
        {
            return RawData[Index];
        }

        T& operator[](size_t Index)
        {
            return RawData[Index];
        }

        /**
         * Attribute(s)
         */

    private:

        T* RawData;

        TStaticArray<T, 4> UnrealData;
    };

    /**
     * Unreal extension of RenderShader implementation for Rive's pixel local storage renderer.
     */
    class FUnrealRiveGradient
#if WITH_RIVE
        final : public rive::lite_rtti_override<rive::RenderShader, FUnrealRiveGradient>
#endif // WITH_RIVE
    {

        /**
         * Structor(s)
         */

    public:

        FUnrealRiveGradient();

        ~FUnrealRiveGradient();

#if WITH_RIVE

    private:

        FUnrealRiveGradient(EPaintType InPaintType,
            FUnrealRiveGradData<rive::ColorInt>&& InColors, // [count]
            FUnrealRiveGradData<float>&& InStops,     // [count]
            size_t InCount,
            float CoeffX,
            float CoeffY,
            float CoeffZ);

        /**
         * Implementation(s)
         */

    public:

        static rive::rcp<FUnrealRiveGradient> MakeLinear(float BeginsFromX,
            float BeginsFromY,
            float EndsAtX,
            float EndsAtY,
            const rive::ColorInt InColors[], // [count]
            const float InStops[],     // [count]
            size_t InCount);

        static rive::rcp<FUnrealRiveGradient> MakeRadial(float CenterX,
            float CenterY,
            float InRadius,
            const rive::ColorInt InColors[], // [count]
            const float InStops[],     // [count]
            size_t InCount);

        EPaintType GetPaintType() const;

        const float* GetCoeffs() const;

        const rive::ColorInt* GetColors() const;

        const float* GetStops() const;

        int GetCount() const;

        /**
         * Attribute(s)
         */

    private:

        EPaintType PaintType; // Specifically, LinearGradient or RadialGradient.

        FUnrealRiveGradData<rive::ColorInt> Colors;

        FUnrealRiveGradData<float> Stops;

        size_t Count;

        TStaticArray<float, 3> Coeffs;

#endif // WITH_RIVE
    };
}
