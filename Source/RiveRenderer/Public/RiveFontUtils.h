// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include <cstdint>
#include <vector>

struct FFontData;

namespace RiveFontUtils
{
/** Largest font file ExtractFontBytes will read from disk. The biggest CJK
 * faces are tens of MB, so this only rejects files that aren't fonts. */
inline constexpr int64 MaxFontFileBytes = 256LL * 1024 * 1024;

/**
 * Resolves the raw TrueType/OpenType bytes behind an Unreal font, in the form
 * rive::CommandQueue::decodeFont expects. Mirrors how Slate's font cache
 * finds the bytes, so it works for every font face loading policy:
 *  - Inline, and any policy in the editor: the bytes held in memory.
 *  - LazyLoad/Stream in a cooked build: the cooked .ufont next to the asset,
 *    unwrapped from its serialized FFontFaceData.
 *  - A raw font file on disk (simulated standalone, legacy filename fonts).
 * WOFF/WOFF2 are rejected: Rive's HarfBuzz decoder can't read them.
 * Reads from disk for non-Inline fonts in cooked builds, so avoid calling it
 * on the game thread there.
 * @param FontData The font to resolve, e.g. FFontData(UFontFace*).
 * @param OutBytes Receives the font bytes on success.
 * @return Whether the bytes were found; failures are logged with the reason.
 */
RIVERENDERER_API bool ExtractFontBytes(
    const FFontData& FontData,
    std::vector<uint8_t>& OutBytes);
} // namespace RiveFontUtils
