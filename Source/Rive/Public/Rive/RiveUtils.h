// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Rive/RiveFile.h"
#include <string>

namespace RiveUtils
{
/**
 * Converts an FString to a UTF-8 encoded std::string
 */
FORCEINLINE std::string ToUTF8(const FString& InString)
{
    FTCHARToUTF8 Convert(*InString);
    return std::string(Convert.Get(), Convert.Length());
}
static FORCEINLINE FString GetPackagePathForFile(const URiveFile* File)
{
    static FString GeneratedFolderPath = TEXT("/Game/__RiveGenerated__/");
    return GeneratedFolderPath + File->GetName();
}

static FORCEINLINE FString GetTempPathForFile(const URiveFile* File)
{
    static FString GeneratedFolderPath = TEXT("/Game/__RiveGenerated__/Tmp/");
    return GeneratedFolderPath + File->GetName();
}
// Reimplementation of ObjectUtils::SanitizeObjectName because we want to
// sanitize even during packaged builds
static FORCEINLINE FString SanitizeObjectName(const FString& InName)
{
    const TCHAR* InvalidChar = INVALID_OBJECTNAME_CHARACTERS;
    FString OutName = InName;
    while (*InvalidChar)
    {
        OutName.ReplaceCharInline(*InvalidChar,
                                  TEXT('_'),
                                  ESearchCase::CaseSensitive);
        ++InvalidChar;
    }
    return OutName;
}
}; // namespace RiveUtils