#pragma once

#include "specialization.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char specialization[] = R"===(layout(constant_id = CLIPPING_SPECIALIZATION_IDX) const bool kEnableClipping = false;
layout(constant_id = CLIP_RECT_SPECIALIZATION_IDX) const bool kEnableClipRect = false;
layout(constant_id = ADVANCED_BLEND_SPECIALIZATION_IDX) const bool kEnableAdvancedBlend = false;
layout(constant_id = EVEN_ODD_SPECIALIZATION_IDX) const bool kEnableEvenOdd = false;
layout(constant_id = NESTED_CLIPPING_SPECIALIZATION_IDX) const bool kEnableNestedClipping = false;
layout(constant_id = HSL_BLEND_MODES_SPECIALIZATION_IDX) const bool kEnableHSLBlendModes = false;

#define _EXPORTED_ENABLE_CLIPPING  kEnableClipping
#define _EXPORTED_ENABLE_CLIP_RECT  kEnableClipRect
#define _EXPORTED_ENABLE_ADVANCED_BLEND  kEnableAdvancedBlend
#define _EXPORTED_ENABLE_EVEN_ODD  kEnableEvenOdd
#define _EXPORTED_ENABLE_NESTED_CLIPPING  kEnableNestedClipping
#define _EXPORTED_ENABLE_HSL_BLEND_MODES  kEnableHSLBlendModes
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive