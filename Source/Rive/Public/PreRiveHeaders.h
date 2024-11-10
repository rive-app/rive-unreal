// Copyright Rive, Inc. All rights reserved.

//  ---- File to be imported before any Rive Header waiting for fix by Rive
// The issue is that the Rive Libraries are currently compiled for debug, and
// check for some macros like NDEBUG and DEBUG to do some assertions. As we
// import the Rive .hpp headers directly from our code, these headers can end up
// being compiled after Unreal has already defined those Macros: NDEBUG could be
// set to 1 in UE while Rive is expecting NDEBUG to be 0, compiling the wrong
// version of RefCnt for example, which will trigger some assertions.

#pragma once

#include "HAL/PreprocessorHelpers.h"

#ifdef NDEBUG
#pragma message("NDEBUG is set to " PREPROCESSOR_TO_STRING(                    \
    NDEBUG) " before including Rive libraries! Undefining NDEBUG...")
// #undef NDEBUG
#endif

#ifdef DEBUG
#pragma message("DEBUG is set to " PREPROCESSOR_TO_STRING(                     \
    DEBUG) " before including Rive libraries!")
#endif
