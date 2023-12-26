// Copyright Rive, Inc. All rights reserved.

#pragma once

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/math/raw_path.hpp"
#include "rive/renderer.hpp"
#include "utils/lite_rtti.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

namespace UE::Rive::Core::Private
{
    class FUnrealRivePath;

    /**
     * Type definition for unique pointer reference to the instance of FUnrealRivePath.
     */
    using FUnrealRivePathPtr = std::unique_ptr<FUnrealRivePath>;

    /**
     *
     */
    class FUnrealRivePath
#if WITH_RIVE
        final : public rive::lite_rtti_override<rive::RenderPath, FUnrealRivePath>
#endif // WITH_RIVE
    {
        /**
         * Structor(s)
         */

    public:

        FUnrealRivePath() = default;

#if WITH_RIVE

        explicit FUnrealRivePath(rive::FillRule InFillMode, rive::RawPath& InRawPath);

    private:

        enum EDirtyComponent
        {
            EDC_PathBounds = 1,

            EDC_UniqueID = 4,

            EDC_All = ~0,
        };

        //~ BEGIN : rive::RenderPath Interface

   public:

       void rewind() override;

       void fillRule(rive::FillRule NewFileMode) override;

       void moveTo(float InX, float InY) override;

       void lineTo(float InX, float InY) override;

       void cubicTo(float InOriginX // Starting Control Point X
           , float InOriginY // Starting Control Point Y
           , float InExtentX // Ending Control Point X
           , float InExtentY // Ending Control Point X
           , float InX, float InY) override;

       void close() override;

       void addPath(CommandPath* InPath, const rive::Mat2D& InPointsMatrix) override;

       void addRenderPath(RenderPath* InPath, const rive::Mat2D& InPointsMatrix) override;

       //~ END : rive::RenderPath Interface

       /**
        * Implementation(s)
        */

    public:

        const rive::RawPath& GetRawPath() const;

        rive::FillRule GetFillMode() const;

        const rive::AABB& GetBounds();

        uint64 GetUniqueID();

        void ClearPathDirt(EDirtyComponent InDirtyComponent = EDC_All);

        bool IsPathDirty(EDirtyComponent InDirtyComponent = EDC_All);

        void MarkPathDirty(EDirtyComponent InDirtyComponent = EDC_All);

        /**
         * Attribute(s)
         */

    private:

        rive::FillRule FillMode = rive::FillRule::nonZero;

        rive::RawPath RawPath;

        rive::AABB Bounds;

        uint64 UniqueID;

        uint32 DirtyFlag = EDC_All;

#endif // WITH_RIVE
    };
}