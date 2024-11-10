// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveAudioEngine.h"
#include "AudioDevice.h"
#include "rive/artboard.hpp"

void URiveAudioEngine::BeginPlay()
{
    if (FAudioDevice* AudioDevice = GetAudioDevice())
    {
        // Make our Rive audio engine
        if (NativeAudioEnginePtr.get() != nullptr)
        {
            NativeAudioEnginePtr->unref();
            NativeAudioEnginePtr = nullptr;
        }
        NumChannels = 2;

        NativeAudioEnginePtr = rive::rcp(
            rive::AudioEngine::Make(NumChannels, AudioDevice->SampleRate));
        Start();
    }

    OnRiveAudioReady.Broadcast();
    Super::BeginPlay();
}

int32 URiveAudioEngine::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
    if (NativeAudioEnginePtr == nullptr)
        return 0;

    TArray<float> AudioData;
    AudioData.AddZeroed(NumSamples);

    if (NativeAudioEnginePtr->readAudioFrames(AudioData.GetData(),
                                              NumSamples / NumChannels,
                                              nullptr))
    {
        FMemory::Memcpy(OutAudio,
                        AudioData.GetData(),
                        NumSamples * sizeof(float));
        return NumSamples;
    }
    return 0;
}