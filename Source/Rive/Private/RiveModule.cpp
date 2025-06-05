// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "RiveModule.h"

#include "Interfaces/IPluginManager.h"
#include "Logs/RiveLog.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "Misc/CoreDelegates.h"
#include "Misc/MessageDialog.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Internationalization/Internationalization.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/runtime_header.hpp"
#include "Tests/JuiceRive.h"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

#define LOCTEXT_NAMESPACE "FRiveModule"

void FRiveModule::StartupModule()
{
    TestRiveIntegration();

    FCoreDelegates::OnFEngineLoopInitComplete.AddLambda([]() {
        static FDelegateHandle TickHandle;
        TickHandle = FSlateApplication::Get().OnPostTick().AddLambda([](float) {
            const TCHAR* msg = TEXT(
                "We're rewriting our Unreal Engine "
                "integration to deliver significantly better "
                "performance, and it's already showing a 4x speed "
                "boost. To focus on this effort, we're temporarily "
                "pausing support and no longer recommending the "
                "current version of the Rive x Unreal plugin, which "
                "was released as an experimental preview.\n\n"
                "More details here: "
                "https://community.rive.app/c/announcements/rive-x-unreal");

            TSharedPtr<SWindow> MainWindow =
                FSlateApplication::Get().GetActiveTopLevelWindow();
            if (MainWindow.IsValid() && MainWindow->IsVisible())
            {
                FSlateApplication::Get().OnPostTick().Remove(TickHandle);

                FMessageDialog::Open(
                    EAppMsgType::Ok,
                    FText::FromString(msg),
                    FText::FromString(TEXT("RIVE ANNOUNCEMENT")));
            }
        });
    });
}

void FRiveModule::ShutdownModule() { ResetAllShaderSourceDirectoryMappings(); }

void FRiveModule::TestRiveIntegration()
{
    bool bIsRiveRuntimeLoaded = false;

    FText RiveRuntimeMsg =
        LOCTEXT("RiveRuntimeMsg", "Failed to load Rive Runtime library.");

#if WITH_RIVE

    rive::BinaryReader JuiceRivReader(
        rive::Span(UE::Rive::Tests::JuiceRivFile,
                   sizeof(&UE::Rive::Tests::JuiceRivFile)));

    rive::RuntimeHeader JuiceRivHeader;

    if (rive::RuntimeHeader::read(JuiceRivReader, JuiceRivHeader))
    {
        FFormatNamedArguments VersionArgs;

        VersionArgs.Add(TEXT("Major"),
                        FText::AsNumber(JuiceRivHeader.majorVersion()));

        VersionArgs.Add(TEXT("Minor"),
                        FText::AsNumber(JuiceRivHeader.minorVersion()));

        VersionArgs.Add(
            TEXT("FileId"),
            FText::AsNumber(JuiceRivHeader.fileId(),
                            &FNumberFormattingOptions().SetUseGrouping(false)));

        RiveRuntimeMsg =
            FText::Format(LOCTEXT("RiveVersionStr",
                                  "Using Rive Runtime : {Major}.{Minor}; Juice "
                                  "Rive File Id : {FileId}"),
                          VersionArgs);

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

IMPLEMENT_MODULE(FRiveModule, Rive)
