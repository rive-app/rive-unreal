// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"
#include "IRiveRenderTarget.h"
#include "RiveRenderCommand.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/refcnt.hpp"
THIRD_PARTY_INCLUDES_END

namespace rive
{
class RiveRenderer;
}

#endif // WITH_RIVE

class UTexture2DDynamic;

class FRiveRenderer;

class FRiveRenderTarget : public IRiveRenderTarget
{
    /**
     * Structor(s)
     */

public:
    FRiveRenderTarget(const TSharedRef<FRiveRenderer>& InRiveRenderer,
                      const FName& InRiveName,
                      UTexture2DDynamic* InRenderTarget);
    virtual ~FRiveRenderTarget() override;

    virtual void Initialize() override;

    virtual void CacheTextureTarget_RenderThread(
        FRHICommandListImmediate& RHICmdList,
        const FTextureRHIRef& InRHIResource) override
    {}

    virtual uint32 GetWidth() const override;
    virtual uint32 GetHeight() const override;
    virtual void SetClearColor(const FLinearColor& InColor) override
    {
        ClearColor = InColor;
    }

    //~ END : IRiveRenderTarget Interface

    /**
     * Implementation(s)
     */

#if WITH_RIVE
    virtual void Submit() override;
    virtual void SubmitAndClear() override;
    virtual void Save() override;
    virtual void Restore() override;
    virtual void Transform(float X1,
                           float Y1,
                           float X2,
                           float Y2,
                           float TX,
                           float TY) override;
    virtual void Translate(const FVector2f& InVector) override;
    virtual void Draw(rive::Artboard* InArtboard) override;
    virtual void Align(const FBox2f& InBox,
                       ERiveFitType InFit,
                       const FVector2f& InAlignment,
                       float InScaleFactor,
                       rive::Artboard* InArtboard) override;
    virtual void Align(ERiveFitType InFit,
                       const FVector2f& InAlignment,
                       float InScaleFactor,
                       rive::Artboard* InArtboard) override;
    virtual FMatrix GetTransformMatrix() const override;
    virtual void RegisterRenderCommand(
        RiveRenderFunction RenderFunction) override;

protected:
    virtual rive::rcp<rive::gpu::RenderTarget> GetRenderTarget() const = 0;
    virtual std::unique_ptr<rive::RiveRenderer> BeginFrame();
    virtual void EndFrame() const;
    virtual void Render_RenderThread(
        FRHICommandListImmediate& RHICmdList,
        const TArray<FRiveRenderCommand>& RiveRenderCommands);
    virtual void Render_Internal(
        const TArray<FRiveRenderCommand>& RiveRenderCommands);
#endif // WITH_RIVE

protected:
    mutable bool bIsCleared = false;
    FLinearColor ClearColor = FLinearColor::Transparent;
    FName RiveName;
    TObjectPtr<UTexture2DDynamic> RenderTarget;
    TArray<FRiveRenderCommand> RenderCommands;
    TSharedPtr<FRiveRenderer> RiveRenderer;
    mutable FDateTime LastResetTime = FDateTime::Now();
    static FTimespan ResetTimeLimit;
};
