/*
 * Copyright 2025 Rive
 */
#include "Ore/ore_bind_group_rhi.hpp"
#include "Ore/ore_bind_group_layout_rhi.hpp"
#include "Ore/ore_context_rhi.hpp"

namespace rive::ore
{
OreBindGroupLayoutRHI::OreBindGroupLayoutRHI(
    OreContextRHI* Context,
    const rive::ore::BindGroupLayoutDesc& desc)
{
    m_groupIndex = desc.groupIndex;
    m_entries.assign(desc.entries, desc.entries + desc.entryCount);
    m_context = Context;
}
} // namespace rive::ore
