// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#if ENGINE_MAJOR_VERSION > 5 ||                                                \
    (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8)
#include "Stats/Stats.h"
#else
#include "Stats/Stats2.h"
#endif

DECLARE_STATS_GROUP(TEXT("RiveEditor"), STATGROUP_RiveEditor, STATCAT_Advanced);
