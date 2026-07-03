/*
 * Copyright 2025 Rive
 */
#include "Ore/ore_pipeline_rhi.hpp"
#include "Ore/ore_bind_group_layout_rhi.hpp"

namespace rive::ore
{

OrePipelineRHI::OrePipelineRHI(const PipelineDesc& desc) :
    lite_rtti_override(desc)
{}

} // namespace rive::ore
