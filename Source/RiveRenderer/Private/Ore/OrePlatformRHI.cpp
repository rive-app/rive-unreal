// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "Ore/OrePlatformRHI.h"

// Default to the desktop-RHI behavior. A platform whose RHI needs one of these
// Ore adjustments sets the corresponding global to true from its own
// (out-of-tree) RHI/module startup code, e.g.:
//
//     GRHIOreNeedsSlopeBiasAdjusted        = true;
//     GRHIOreNeedsResolveTargetFlag        = true;
//     GRHIOreNeedsDepthStencilShadowFormat = true;
//     GRHIOreNeedsReflectionSlotRemap      = true;
//     GRHIOreNeedsBindlessParameters       = true;
//
// matching how the engine's GRHI* capability globals are set by each RHI's
// init.
bool GRHIOreNeedsSlopeBiasAdjusted = false;
bool GRHIOreNeedsResolveTargetFlag = false;
bool GRHIOreNeedsDepthStencilShadowFormat = false;
bool GRHIOreNeedsReflectionSlotRemap = false;
bool GRHIOreNeedsBindlessParameters = false;
