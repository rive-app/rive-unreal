// Copyright Rive, Inc. All rights reserved.

#include "Rive/Assets/RiveAudioAsset.h"

THIRD_PARTY_INCLUDES_START
#include "rive/assets/audio_asset.hpp"
#include "rive/audio/audio_source.hpp"
THIRD_PARTY_INCLUDES_END

URiveAudioAsset::URiveAudioAsset() { Type = ERiveAssetType::Audio; }

bool URiveAudioAsset::LoadNativeAssetBytes(
    rive::FileAsset& InAsset,
    rive::Factory* InRiveFactory,
    const rive::Span<const uint8>& AssetBytes)
{
    rive::SimpleArray<uint8_t> Data =
        rive::SimpleArray(AssetBytes.data(), AssetBytes.count());
    rive::AudioSource* AudioSource = new rive::AudioSource(Data);
    rive::AudioAsset* AudioAsset = InAsset.as<rive::AudioAsset>();
    AudioAsset->audioSource(ref_rcp(AudioSource));
    NativeAsset = AudioAsset;
    return true;
}
