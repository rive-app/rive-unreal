#include "Rive/Assets/RiveAsset.h"

#include "Logs/RiveLog.h"
#include "rive/factory.hpp"

void URiveAsset::PostLoad()
{
	UObject::PostLoad();

	// NativeAsset.Bytes = &NativeAssetBytes;
}

void URiveAsset::LoadFromDisk()
{
	if (!FFileHelper::LoadFileToArray(NativeAssetBytes, *AssetPath))
	{
		UE_LOG(LogRive, Error, TEXT("Could not load Asset: %s at path %s"), *Name, *AssetPath);
	}
}

bool URiveAsset::DecodeNativeAsset(rive::FileAsset& InAsset, rive::Factory* InRiveFactory, const rive::Span<const uint8>& AssetBytes)
{
	switch(Type)
	{
	case ERiveAssetType::Font:
		return DecodeFontAsset(InAsset, InRiveFactory, AssetBytes);
	case ERiveAssetType::Image:
		return DecodeImageAsset(InAsset, InRiveFactory, AssetBytes);
	}

	return false;
}

bool URiveAsset::DecodeImageAsset(rive::FileAsset& InAsset, rive::Factory* InRiveFactory, const rive::Span<const uint8>& AssetBytes)
{
	rive::rcp<rive::RenderImage> DecodedImage = InRiveFactory->decodeImage(AssetBytes);

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

bool URiveAsset::DecodeFontAsset(rive::FileAsset& InAsset, rive::Factory* InRiveFactory,
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
