// Copyright Rive, Inc. All rights reserved.

#include "Rive/Assets/RiveAsset.h"

#include "Logs/RiveLog.h"
#include "Misc/FileHelper.h"
#include "rive/factory.hpp"
#include "rive/assets/audio_asset.hpp"
#include "rive/audio/audio_source.hpp"

void URiveAsset::PostLoad()
{
	UObject::PostLoad();

	// Older version of UE Rive used these values as enum values
	// Newer version of UE Rive doesn't use these values as enum values because the type values are beyond uint8 space
	// This ensures older rive assets will still work, by converting old values to new values
	switch((int)Type)
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

void URiveAsset::LoadFromDisk()
{
	if (!FFileHelper::LoadFileToArray(NativeAssetBytes, *AssetPath))
	{
		UE_LOG(LogRive, Error, TEXT("Could not load Asset: %s at path %s"), *Name, *AssetPath);
	}
}

bool URiveAsset::LoadNativeAsset(rive::FileAsset& InAsset, rive::Factory* InRiveFactory, const rive::Span<const uint8>& AssetBytes)
{
	switch(Type)
	{
	case ERiveAssetType::Font:
		return DecodeFontAsset(InAsset, InRiveFactory, AssetBytes);
	case ERiveAssetType::Image:
		return DecodeImageAsset(InAsset, InRiveFactory, AssetBytes);
	case ERiveAssetType::Audio:
		return LoadAudioAsset(InAsset, InRiveFactory, AssetBytes);
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

bool URiveAsset::LoadAudioAsset(rive::FileAsset& InAsset, rive::Factory* InRiveFactory, const rive::Span<const uint8>& AssetBytes)
{
	rive::SimpleArray<uint8_t> Data = rive::SimpleArray(AssetBytes.data(), AssetBytes.count());
	rive::AudioSource* AudioSource = new rive::AudioSource(Data);
	rive::AudioAsset* AudioAsset = InAsset.as<rive::AudioAsset>();
	AudioAsset->audioSource(ref_rcp(AudioSource));
	NativeAsset = AudioAsset;
	return true;
}