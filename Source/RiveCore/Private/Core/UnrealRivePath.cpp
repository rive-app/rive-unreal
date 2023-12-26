// Copyright Rive, Inc. All rights reserved.

#include "Core/UnrealRivePath.h"

#if WITH_RIVE

UE::Rive::Core::Private::FUnrealRivePath::FUnrealRivePath(rive::FillRule InFillMode, rive::RawPath& InRawPath)
    : FillMode(InFillMode)
{
    RawPath.swap(InRawPath);

    RawPath.pruneEmptySegments();
}

void UE::Rive::Core::Private::FUnrealRivePath::rewind()
{
    RawPath.rewind();

    MarkPathDirty();
}

void UE::Rive::Core::Private::FUnrealRivePath::fillRule(rive::FillRule NewFileMode)
{
    FillMode = NewFileMode;
}

void UE::Rive::Core::Private::FUnrealRivePath::moveTo(float InX, float InY)
{
    RawPath.moveTo(InX, InY);

    MarkPathDirty();
}

void UE::Rive::Core::Private::FUnrealRivePath::lineTo(float InX, float InY)
{
    // Make sure to start a new contour, even if this line is empty.
    RawPath.injectImplicitMoveIfNeeded();

    rive::Vec2D NewPoint = { InX, InY };

    if (RawPath.points().back() != NewPoint)
    {
        RawPath.line(NewPoint);
    }

    MarkPathDirty();
}

void UE::Rive::Core::Private::FUnrealRivePath::cubicTo(float InOriginX, float InOriginY, float InExtentX, float InExtentY, float InX, float InY)
{
    // Make sure to start a new contour, even if this cubic is empty.
    RawPath.injectImplicitMoveIfNeeded();

    rive::Vec2D OriginPoint = { InOriginX, InOriginY };

    rive::Vec2D ExtentPoint = { InExtentX, InExtentY };

    rive::Vec2D NewPoint = { InX, InY };

    if (RawPath.points().back() != OriginPoint || OriginPoint != ExtentPoint || ExtentPoint != NewPoint)
    {
        RawPath.cubic(OriginPoint, ExtentPoint, NewPoint);
    }

    MarkPathDirty();
}

void UE::Rive::Core::Private::FUnrealRivePath::close()
{
    RawPath.close();

    MarkPathDirty();
}

void UE::Rive::Core::Private::FUnrealRivePath::addPath(CommandPath* InPath, const rive::Mat2D& InPointsMatrix)
{
    addRenderPath(InPath->renderPath(), InPointsMatrix);
}

void UE::Rive::Core::Private::FUnrealRivePath::addRenderPath(RenderPath* InPath, const rive::Mat2D& InPointsMatrix)
{
    FUnrealRivePath* UnrealRivePath = static_cast<FUnrealRivePath*>(InPath);

    rive::RawPath::Iter TransformedPathIter = RawPath.addPath(UnrealRivePath->RawPath, &InPointsMatrix);

    if (InPointsMatrix != rive::Mat2D())
    {
        // Prune any segments that became empty after the transform.
        RawPath.pruneEmptySegments(TransformedPathIter);
    }

    MarkPathDirty();
}

const rive::RawPath& UE::Rive::Core::Private::FUnrealRivePath::GetRawPath() const
{
    return RawPath;
}

rive::FillRule UE::Rive::Core::Private::FUnrealRivePath::GetFillMode() const
{
    return FillMode;
}

const rive::AABB& UE::Rive::Core::Private::FUnrealRivePath::GetBounds()
{
    if (IsPathDirty(EDC_PathBounds))
    {
        Bounds = RawPath.bounds();

        ClearPathDirt(EDC_PathBounds);
    }

    return Bounds;
}

uint64 UE::Rive::Core::Private::FUnrealRivePath::GetUniqueID()
{
    static std::atomic<uint64_t> UniqueIDCounter = 0;

    if (IsPathDirty(EDC_UniqueID))
    {
        UniqueID = ++UniqueIDCounter;

        ClearPathDirt(EDC_UniqueID);
    }

    return UniqueID;
}

void UE::Rive::Core::Private::FUnrealRivePath::ClearPathDirt(EDirtyComponent InDirtyComponent)
{
    DirtyFlag &= ~InDirtyComponent;
}

bool UE::Rive::Core::Private::FUnrealRivePath::IsPathDirty(EDirtyComponent InDirtyComponent)
{
    return (DirtyFlag & InDirtyComponent) == InDirtyComponent;
}

void UE::Rive::Core::Private::FUnrealRivePath::MarkPathDirty(EDirtyComponent InDirtyComponent)
{
    DirtyFlag = InDirtyComponent;
}

#endif // WITH_RIVE
