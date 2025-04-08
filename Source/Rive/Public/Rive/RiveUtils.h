// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include <string>

namespace RiveUtils
{
/**
 * Converts an FString to a UTF-8 encoded std::string
 */
inline std::string ToUTF8(const FString& InString)
{
    FTCHARToUTF8 Convert(*InString);
    return std::string(Convert.Get(), Convert.Length());
}
}; // namespace RiveUtils