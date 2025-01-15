// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Stats/Stats.h"
/*
 * Stats group for game objects that rive uses, like RiveTextureObject
 */
DECLARE_STATS_GROUP(TEXT("Rive"), STATGROUP_Rive, STATCAT_Advanced);
/*
 * Stats group for non gpu based stats in the rive renderer. For future use
 * with render graph style api for rive.
 *
 * Gpu stats are a little different and can not have their own group, otherwise
 * they would have used this as well
 */
DECLARE_STATS_GROUP(TEXT("RiveRenderer"),
                    STATGROUP_RiveRenderer,
                    STATCAT_Advanced);

/*
 * Stats group for all editor specific rive stats
 */
DECLARE_STATS_GROUP(TEXT("RiveEditor"), STATGROUP_RiveEditor, STATCAT_Advanced);
