// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRiveRenderer, Display, All);

class FDebugLogger
{
private:
    const int SpacesPerIndent = 2;
    int IndentLevel = 0;
    FString IndentStr{};

public:
    int CurrentLevel() const { return IndentLevel; }
    const FString& CurrentIndent() const { return IndentStr; }
    void Indent()
    {
        ++IndentLevel;
        IndentStr.Append(TEXT(".")).Append(TEXT("          "), SpacesPerIndent);
    }
    void UnIndent()
    {
        --IndentLevel;
        IndentStr.RemoveAt(IndentStr.Len() - SpacesPerIndent - 1,
                           SpacesPerIndent + 1);
    }
    static FDebugLogger& Get()
    {
        static FDebugLogger Logger = {};
        return Logger;
    }

    static const TCHAR* Ind() { return *Get().CurrentIndent(); }

    static FString CurrentThread()
    {
        if (IsInGameThread())
        {
            return {TEXT("GameThread")};
        }
        else if (IsInRHIThread())
        {
            return {TEXT("RHIThread")};
        }
        else if (IsInRenderingThread())
        {
            return {TEXT("RenderThread")};
        }
        return {TEXT("UnknownThread")};
    }
};

class FScopeLogIndent
{
public:
    UE_NODISCARD_CTOR FScopeLogIndent() { FDebugLogger::Get().Indent(); }
    ~FScopeLogIndent() { FDebugLogger::Get().UnIndent(); }
};

/**
 * Logs the current function name with thread information and indents the next
 * call to RIVE_DEBUG_
 */
#define RIVE_DEBUG_FUNCTION_INDENT                                             \
    UE_LOG(LogRiveRenderer,                                                    \
           Verbose,                                                            \
           TEXT("[%s]%s-- %hs"),                                               \
           *FDebugLogger::CurrentThread(),                                     \
           FDebugLogger::Ind(),                                                \
           __FUNCTION__);                                                      \
    FScopeLogIndent MacroLogIndent{};

/**
 * Logs the the given message indented with thread information
 */
#define RIVE_DEBUG_VERBOSE(Format, ...)                                        \
    UE_LOG(LogRiveRenderer,                                                    \
           Verbose,                                                            \
           TEXT("[%s]%s %s"),                                                  \
           *FDebugLogger::CurrentThread(),                                     \
           FDebugLogger::Ind(),                                                \
           *FString::Printf(TEXT(Format), ##__VA_ARGS__));

/**
 * Logs the the given message indented with thread information
 */
#define RIVE_DEBUG_ERROR(Format, ...)                                          \
    UE_LOG(LogRiveRenderer,                                                    \
           Error,                                                              \
           TEXT("[%s]%s %s"),                                                  \
           *FDebugLogger::CurrentThread(),                                     \
           FDebugLogger::Ind(),                                                \
           *FString::Printf(TEXT(Format), ##__VA_ARGS__));
