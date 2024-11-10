// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Components/SynthComponent.h"
#include "CoreMinimal.h"
#include "Sound/SoundWaveProcedural.h"
#include "rive/audio/audio_engine.hpp"
#include "RiveAudioEngine.generated.h"

namespace rive
{
class Artboard;
}

UCLASS(ClassGroup = (Rive), Meta = (BlueprintSpawnableComponent))
class RIVE_API URiveAudioEngine : public USynthComponent
{
    GENERATED_BODY()
public:
    DECLARE_MULTICAST_DELEGATE(FOnRiveAudioEngineReadyEvent)

    virtual void BeginPlay() override;

    // Event called after BeginPlay, and after NativeAudioEnginePtr has been
    // made This can be used if you expect initialization of a user of this
    // system to initialize before this is ready
    FOnRiveAudioEngineReadyEvent OnRiveAudioReady;

    virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

    rive::rcp<rive::AudioEngine> GetNativeAudioEngine()
    {
        return NativeAudioEnginePtr;
    }

private:
    rive::rcp<rive::AudioEngine> NativeAudioEnginePtr = nullptr;
};
