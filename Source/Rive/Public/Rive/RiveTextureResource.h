// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "TextureResource.h"

class URiveTexture;

/**
 * Custom Texture Resource for Rive file
 */
class RIVE_API FRiveTextureResource : public FTextureResource
{
public:
    FRiveTextureResource(URiveTexture* Owner);

    virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
    virtual void ReleaseRHI() override;
    virtual uint32 GetSizeX() const override;
    virtual uint32 GetSizeY() const override;

    SIZE_T GetResourceSize();

private:
    TObjectPtr<URiveTexture> RiveTexture;
};
