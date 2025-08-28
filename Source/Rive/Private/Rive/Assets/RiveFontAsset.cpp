// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "Rive/Assets/RiveFontAsset.h"

#include "IRiveRendererModule.h"
#include "RiveRenderer.h"
#include "Engine/FontFace.h"
#include "Logs/RiveLog.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/renderer/render_context.hpp"
THIRD_PARTY_INCLUDES_END

URiveFontAsset::URiveFontAsset() { Type = ERiveAssetType::Font; }

void URiveFontAsset::LoadFontFace(UFontFace* InFontFace)
{
    if (!InFontFace || Type != ERiveAssetType::Font)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("LoadFontFace: Invalid or missing FontFace."));
        return;
    }

    if (InFontFace->LoadingPolicy != EFontLoadingPolicy::Inline)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("LoadFontFace: trying to load a font that isn't marked "
                    "Inline, this will fail. "
                    "Please change the FontFace to use the 'Inline' Loading "
                    "Policy."));
        return;
    }

    if (InFontFace->FontFaceData->HasData() &&
        InFontFace->FontFaceData->GetData().Num() > 0)
    {
        LoadFontBytes(InFontFace->FontFaceData->GetData());
    }
    else
    {
        UE_LOG(LogRive,
               Warning,
               TEXT("LoadFontFace: FontFace has no data to decode."));
    }
}

void URiveFontAsset::LoadFontBytes(const TArray<uint8>& InBytes)
{
    FRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();

    // We'll copy InBytes into the lambda because there's no guarantee they'll
    // exist by the time it's hit

    auto& CommandBuilder = RiveRenderer->GetCommandBuilder();
    CommandBuilder.RunOnce([NativeAsset = NativeAsset, RiveRenderer, InBytes](
                               rive::CommandServer*) {
        rive::gpu::RenderContext* RenderContext =
            RiveRenderer->GetRenderContext();

        if (ensure(RenderContext))
        {
            auto DecodedFont = RenderContext->decodeFont(
                rive::make_span(InBytes.GetData(), InBytes.Num()));

            if (DecodedFont == nullptr)
            {
                UE_LOG(LogRive,
                       Error,
                       TEXT("LoadFontBytes: Could not decode font bytes"));
                return;
            }

            rive::FontAsset* FontAsset = NativeAsset->as<rive::FontAsset>();
            FontAsset->font(DecodedFont);
        }
    });
}

bool URiveFontAsset::LoadNativeAssetBytes(
    rive::FileAsset& InAsset,
    rive::Factory* InRiveFactory,
    const rive::Span<const uint8>& AssetBytes)
{
    rive::rcp<rive::Font> DecodedFont = InRiveFactory->decodeFont(AssetBytes);

    if (DecodedFont == nullptr)
    {
        UE_LOG(LogRive, Error, TEXT("Could not decode font asset: %s"), *Name);
        return false;
    }

    rive::FontAsset* FontAsset = InAsset.as<rive::FontAsset>();
    FontAsset->font(DecodedFont);
    NativeAsset = FontAsset;
    return true;
}
