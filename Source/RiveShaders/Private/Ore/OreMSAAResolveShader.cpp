/*
 * Copyright 2025 Rive
 */

#include "Ore/OreMSAAResolveShader.h"

IMPLEMENT_GLOBAL_SHADER(FOreMSAAResolveVS,
                        "/Plugin/Rive/Private/Ore/OreMSAAResolve.usf",
                        "MainVS",
                        SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FOreMSAAResolvePS,
                        "/Plugin/Rive/Private/Ore/OreMSAAResolve.usf",
                        "MainPS",
                        SF_Pixel);
