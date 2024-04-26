/*
 * Copyright 2023 Rive
 */

#pragma once

#define NOMINMAX

// d3d11.h defines these, which are also defined in the Windows SDK used in this engine version
#undef D3D10_ERROR_FILE_NOT_FOUND
#undef D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS
#undef D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS
#undef D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS
#undef D3D11_ERROR_FILE_NOT_FOUND
#undef D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD

#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#define VERIFY_OK(CODE)                                                                            \
    {                                                                                              \
        HRESULT hr = (CODE);                                                                       \
        if (hr != S_OK)                                                                            \
        {                                                                                          \
            fprintf(stderr,                                                                        \
                    __FILE__ ":%i: D3D error 0x%lx: %s\n",                                         \
                    static_cast<int>(__LINE__),                                                    \
                    hr,                                                                            \
                    #CODE);                                                                        \
            exit(-1);                                                                              \
        }                                                                                          \
    }
