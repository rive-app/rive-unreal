// Copyright Rive, Inc. All rights reserved.

#include "RiveRenderCommand.h"

#include "RiveCore/Public/PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#include "rive/artboard.hpp"
#include "rive/renderer.hpp"
THIRD_PARTY_INCLUDES_END

rive::Mat2D FRiveRenderCommand::GetSaved2DTransform() const
{
	switch (Type)
	{
	case ERiveRenderCommandType::Transform:
		return rive::Mat2D(X, Y, X2, Y2, TX, TY);
	case ERiveRenderCommandType::AlignArtboard:
		return rive::computeAlignment(
			static_cast<rive::Fit>(FitType),
			rive::Alignment(X, Y),
			rive::AABB(TX, TY, X2, Y2),
			NativeArtboard->bounds());
	case ERiveRenderCommandType::Translate:
		return rive::Mat2D(1.f, 0.f, 0.f, 1.f, TX, TY);
	default: ;
		return rive::Mat2D();
	}
}
