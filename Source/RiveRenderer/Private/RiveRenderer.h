// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRenderer.h"

namespace UE::Rive::Renderer::Private
{
    class FRiveRenderer : public IRiveRenderer
    {
        //~ BEGIN : IRiveRenderer Interface

    public:

        virtual void QueueTextureRendering(TObjectPtr<URiveFile> InRiveFile) override;

        virtual void DebugColorDraw(UTextureRenderTarget2D* InTexture, const FLinearColor DebugColor) override;

        //~ END : IRiveRenderer Interface
    };
}