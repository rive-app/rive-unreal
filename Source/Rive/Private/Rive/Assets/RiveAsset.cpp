// Copyright Rive, Inc. All rights reserved.

#include "Rive/Assets/RiveAsset.h"

void URiveAsset::PostLoad()
{
    UObject::PostLoad();

    // Older version of UE Rive used these values as enum values
    // Newer version of UE Rive doesn't use these values as enum values because
    // the type values are beyond uint8 space This ensures older rive assets
    // will still work, by converting old values to new values
    switch ((int)Type)
    {
        case 103:
            Type = ERiveAssetType::FileBase;
            break;
        case 105:
            Type = ERiveAssetType::Image;
            break;
        case 141:
            Type = ERiveAssetType::Font;
            break;
    }
}
