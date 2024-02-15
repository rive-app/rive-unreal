#pragma once
#include "CoreMinimal.h"
#include "RiveTypes.generated.h"

UENUM(BlueprintType)
enum class ERiveFitType : uint8
{
	Fill = 0,
	Contain = 1,
	Cover = 2,
	FitWidth = 3,
	FitHeight = 4,
	None = 5,
	ScaleDown = 6
};

UENUM(BlueprintType)
enum class ERiveAlignment : uint8
{
	TopLeft = 0,
	TopCenter = 1,
	TopRight = 2,
	CenterLeft = 3,
	Center = 4,
	CenterRight = 5,
	BottomLeft = 6,
	BottomCenter = 7,
	BottomRight = 8,
};

UENUM(BlueprintType)
enum class ERiveBlendMode : uint8
{
	SE_BLEND_Opaque = 0 UMETA(DisplayName = "Opaque"),
	SE_BLEND_Masked UMETA(DisplayName = "Masked"),
	SE_BLEND_Translucent UMETA(DisplayName = "Translucent"),
	SE_BLEND_Additive UMETA(DisplayName = "Additive"),
	SE_BLEND_Modulate UMETA(DisplayName = "Modulate"),
	SE_BLEND_MaskedDistanceField UMETA(DisplayName = "Masked Distance Field"),
	SE_BLEND_MaskedDistanceFieldShadowed UMETA(DisplayName = "Masked Distance Field Shadowed"),
	SE_BLEND_TranslucentDistanceField UMETA(DisplayName = "Translucent Distance Field"),
	SE_BLEND_TranslucentDistanceFieldShadowed UMETA(DisplayName = "Translucent Distance Field Shadowed"),
	SE_BLEND_AlphaComposite UMETA(DisplayName = "Alpha Composite"),
	SE_BLEND_AlphaHoldout UMETA(DisplayName = "Alpha Holdout"),
};
