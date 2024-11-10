// Copyright Rive, Inc. All rights reserved.

#include "Rive/Assets/RiveImageAsset.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "Logs/RiveLog.h"

#include "Engine/Texture2D.h"
#include "Engine/Texture.h"
#include "TextureResource.h"
#include "rive/renderer/render_context_helper_impl.hpp"
#include "rive/renderer/rive_render_image.hpp"

THIRD_PARTY_INCLUDES_START
#include "rive/factory.hpp"
#include "rive/renderer/render_context.hpp"
THIRD_PARTY_INCLUDES_END

namespace UE::Private::RiveImageAsset
{
void GetTextureData(UTexture2D* Texture, TArray<uint8>& OutData)
{
    if (!Texture)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid Texture."));
        return;
    }

    // Access the platform data
    FTexturePlatformData* PlatformData = Texture->GetPlatformData();
    if (!PlatformData || PlatformData->Mips.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No platform data available."));
        return;
    }

    if (PlatformData->Mips.Num() > 1)
    {
        UE_LOG(LogTemp,
               Warning,
               TEXT("Can't load a texture with Mip count > 1"));
        return;
    }

    // Get the mip data from the first mip level
    FTexture2DMipMap& Mip = PlatformData->Mips[0];
    if (const uint8* MipData =
            static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_ONLY)))
    {
        // Copy data to output array
        const uint32 MipSize = Mip.SizeX * Mip.SizeY * sizeof(FColor);
        OutData.SetNumUninitialized(MipSize);
        FMemory::Memcpy(OutData.GetData(), MipData, MipSize);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No texture data available."));
    }

    Mip.BulkData.Unlock();
}
} // namespace UE::Private::RiveImageAsset

URiveImageAsset::URiveImageAsset() { Type = ERiveAssetType::Image; }

void URiveImageAsset::LoadTexture(UTexture2D* InTexture)
{
    if (!InTexture)
        return;

    { // Ensure we have a single mip
        int32 MipCount = InTexture->GetNumMips();
        if (MipCount != 1)
        {
            UE_LOG(LogRive,
                   Error,
                   TEXT("LoadTexture: Texture being loaded needs to have 1 mip "
                        "level to load. You "
                        "can do this by either setting 'Mip Gen Settings' to "
                        "'NoMipMaps', or 'Mip "
                        "Gen Settings' to 'FromTextureGroup' AND 'Texture "
                        "Group' set to 'UI'"));
            return;
        }
    }

    { // Ensure our compression is simple RGBA
        if (InTexture->CompressionSettings !=
            TC_EditorIcon) // TC_EditorIcon is RGBA
        {
            UE_LOG(LogRive,
                   Error,
                   TEXT("LoadTexture: Texture needs to be set to have a "
                        "'CompressionSetting' of "
                        "'UserInterface2D'"));
            return;
        }
    }

    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();

    RiveRenderer->CallOrRegister_OnInitialized(
        IRiveRenderer::FOnRendererInitialized::FDelegate::CreateLambda(
            [this, InTexture](IRiveRenderer* RiveRenderer) {
                rive::gpu::RenderContext* RenderContext;
                {
                    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
                    RenderContext = RiveRenderer->GetRenderContext();
                }

                if (ensure(RenderContext))
                {
                    TArray<uint8> ImageData;
                    UE::Private::RiveImageAsset::GetTextureData(InTexture,
                                                                ImageData);

                    if (ImageData.IsEmpty())
                    {
                        UE_LOG(LogRive,
                               Error,
                               TEXT("LoadTexture: Could not get raw bitmap "
                                    "data from Texture."));
                        return;
                    }

                    TArray64<uint8> CompressedImage;
                    FImageView ImageView = FImageView(ImageData.GetData(),
                                                      InTexture->GetSizeX(),
                                                      InTexture->GetSizeY(),
                                                      ERawImageFormat::BGRA8);
                    IImageWrapperModule& ImageWrapperModule =
                        FModuleManager::LoadModuleChecked<IImageWrapperModule>(
                            FName("ImageWrapper"));
                    ImageWrapperModule.CompressImage(CompressedImage,
                                                     EImageFormat::PNG,
                                                     ImageView,
                                                     100);
                    rive::rcp<rive::RenderImage> RenderImage =
                        RenderContext->decodeImage(
                            rive::make_span(CompressedImage.GetData(),
                                            CompressedImage.Num()));
                    NativeAsset->as<rive::ImageAsset>()->renderImage(
                        RenderImage);
                }
            }));
}

void URiveImageAsset::LoadImageBytes(const TArray<uint8>& InBytes)
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();

    // We'll copy InBytes into the lambda because there's no guarantee they'll
    // exist by the time it's hit
    RiveRenderer->CallOrRegister_OnInitialized(
        IRiveRenderer::FOnRendererInitialized::FDelegate::CreateLambda(
            [this, InBytes](IRiveRenderer* RiveRenderer) {
                rive::gpu::RenderContext* RenderContext;
                {
                    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
                    RenderContext = RiveRenderer->GetRenderContext();
                }

                if (ensure(RenderContext))
                {
                    auto DecodedImage = RenderContext->decodeImage(
                        rive::make_span(InBytes.GetData(), InBytes.Num()));

                    if (DecodedImage == nullptr)
                    {
                        UE_LOG(LogRive,
                               Error,
                               TEXT("LoadImageBytes: Could not decode image "
                                    "bytes"));
                        return;
                    }

                    rive::ImageAsset* ImageAsset =
                        NativeAsset->as<rive::ImageAsset>();
                    ImageAsset->renderImage(DecodedImage);
                }
            }));
}

bool URiveImageAsset::LoadNativeAssetBytes(
    rive::FileAsset& InAsset,
    rive::Factory* InRiveFactory,
    const rive::Span<const uint8>& AssetBytes)
{
    rive::rcp<rive::RenderImage> DecodedImage =
        InRiveFactory->decodeImage(AssetBytes);

    if (DecodedImage == nullptr)
    {
        UE_LOG(LogRive, Error, TEXT("Could not decode image asset: %s"), *Name);
        return false;
    }

    rive::ImageAsset* ImageAsset = InAsset.as<rive::ImageAsset>();
    ImageAsset->renderImage(DecodedImage);
    NativeAsset = ImageAsset;
    return true;
}
