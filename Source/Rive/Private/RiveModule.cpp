// Copyright Rive, Inc. All rights reserved.

#include "RiveModule.h"

#include "Interfaces/IPluginManager.h"
#include "Logs/RiveLog.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/runtime_header.hpp"
#include "Tests/JuiceRive.h"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

#define LOCTEXT_NAMESPACE "FRiveModule"

void UE::Rive::Private::FRiveModule::StartupModule()
{
    TestRiveIntegration();

    const FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("Rive"))->GetBaseDir(), TEXT("Shaders"));
    AddShaderSourceDirectoryMapping(TEXT("/Plugin/Rive"), PluginShaderDir);
}

void UE::Rive::Private::FRiveModule::ShutdownModule()
{
}

void UE::Rive::Private::FRiveModule::TestRiveIntegration()
{
    bool bIsRiveRuntimeLoaded = false;

    FText RiveRuntimeMsg = LOCTEXT("RiveRuntimeMsg", "Failed to load Rive Runtime library.");

#if WITH_RIVE

    rive::BinaryReader JuiceRivReader(rive::Span(Tests::JuiceRivFile, sizeof(&Tests::JuiceRivFile)));

    rive::RuntimeHeader JuiceRivHeader;

    if (rive::RuntimeHeader::read(JuiceRivReader, JuiceRivHeader))
    {
        FFormatNamedArguments VersionArgs;

        VersionArgs.Add(TEXT("Major"), FText::AsNumber(JuiceRivHeader.majorVersion()));

        VersionArgs.Add(TEXT("Minor"), FText::AsNumber(JuiceRivHeader.minorVersion()));

        VersionArgs.Add(TEXT("FileId"), FText::AsNumber(JuiceRivHeader.fileId(), &FNumberFormattingOptions().SetUseGrouping(false)));

        RiveRuntimeMsg = FText::Format(LOCTEXT("RiveVersionStr", "Using Rive Runtime : {Major}.{Minor}; Juice Rive File Id : {FileId}"), VersionArgs);

        bIsRiveRuntimeLoaded = true;
    }

#endif // WITH_RIVE

    if (bIsRiveRuntimeLoaded)
    {
        UE_LOG(LogRive, Display, TEXT("%s"), *RiveRuntimeMsg.ToString());
    }
    else
    {
        UE_LOG(LogRive, Error, TEXT("%s"), *RiveRuntimeMsg.ToString());
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(UE::Rive::Private::FRiveModule, Rive)

