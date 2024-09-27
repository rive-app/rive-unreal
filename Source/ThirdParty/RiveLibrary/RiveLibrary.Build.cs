// Copyright Rive, Inc. All rights reserved.
#if UE_5_0_OR_LATER
using EpicGames.Core;
#else
using Tools.DotNETCommon;
#endif
using System.IO;
using UnrealBuildTool;

public class RiveLibrary : ModuleRules
{
    public RiveLibrary(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        bool bIsPlatformAdded = false;

        PrivateDependencyModuleNames.Add("Vulkan");

        AddEngineThirdPartyPrivateStaticDependencies(Target, "Vulkan");

        AddEngineThirdPartyPrivateStaticDependencies(Target, "zlib");

        string rootDir = ModuleDirectory;
        string includePath = Path.Combine(rootDir, "Includes");

        PublicSystemIncludePaths.Add(includePath);

        // NOTE: Incase if needed, otherwise feel free to remove it.
        // Unreal "Debug" configs don't set bDebugBuildsActuallyUseDebugCRT, so we never actually use the debug libs
        // https://dev.epicgames.com/documentation/en-us/unreal-engine/build-configuration-for-unreal-engine?application_version=5.1
        bool bDebug = (Target.Configuration == UnrealTargetConfiguration.Debug && Target.bDebugBuildsActuallyUseDebugCRT);

        string libSuffix = bDebug ? "_d" : "";
        string libDirectory = "";
        string extension = "";
        string libPrefix = "";

        if (Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
        {
            extension = ".lib";
            AddEngineThirdPartyPrivateStaticDependencies(Target, "DX9");
            AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11");
            // AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");

            PublicSystemLibraries.Add("d3dcompiler.lib");

            libDirectory = Path.Combine(rootDir, "Libraries", "Win64");

            bIsPlatformAdded = true;
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            libPrefix = "lib";
            extension = ".a";
            libDirectory = Path.Combine(rootDir, "Libraries", "Mac");
#if UE_5_0_OR_LATER
            if (Target.Architecture == UnrealArch.Arm64)
#else
            if (Target.Architecture.Contains("arm64"))
#endif
            {
                libDirectory = Path.Combine(libDirectory, "Mac");
                PublicDefinitions.Add("WITH_RIVE_MAC_ARM64 = 1");
            }
            else
            {
                libDirectory = Path.Combine(libDirectory, "Intel");
                PublicDefinitions.Add("WITH_RIVE_MAC_INTEL = 1");
            }

            bIsPlatformAdded = true;
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            libPrefix = "lib";
            libDirectory = Path.Combine(rootDir, "Libraries", "IOS");
#if UE_5_0_OR_LATER
            extension = (Target.Architecture == UnrealArch.IOSSimulator) ? ".sim.a" : ".a";
#else
            extension = (Target.Architecture.Contains("sim")) ? ".sim.a" : ".a";
#endif
            bIsPlatformAdded = true;
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            PrivateDependencyModuleNames.Add("OpenGLDrv");
            AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenGL");

            libDirectory = Path.Combine(rootDir, "Libraries", "Android");
            extension = ".a";

            PublicRuntimeLibraryPaths.Add(libDirectory);

            PrecompileForTargets = PrecompileTargetsType.None;
            bIsPlatformAdded = true;

            string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
            AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(PluginPath, "RiveLibrary_APL.xml"));
        }
        else if (Target.IsInPlatformGroup(UnrealPlatformGroup.Unix))
        {
            libDirectory = Path.Combine(rootDir, "Libraries", "Unix");
            extension = ".a";
            // NOTE : Link Unix Libraries

            bIsPlatformAdded = true;
        }

        if (bIsPlatformAdded)
        {
            PublicAdditionalLibraries.AddRange(new string[]
            {
                Path.Combine(libDirectory, libPrefix+ "rive_libwebp" + libSuffix +  extension),
                Path.Combine(libDirectory, libPrefix+ "rive_sheenbidi" + libSuffix + extension),
                Path.Combine(libDirectory, libPrefix+ "rive_harfbuzz" + libSuffix + extension),
                Path.Combine(libDirectory, libPrefix+ "rive_libwebp" + libSuffix + extension),
                Path.Combine(libDirectory, libPrefix+ "rive_libpng" + libSuffix + extension),
                Path.Combine(libDirectory, libPrefix+ "rive_libjpeg" + libSuffix + extension),
                Path.Combine(libDirectory, libPrefix+ "rive_decoders" + libSuffix + extension),
                Path.Combine(libDirectory, libPrefix+ "rive_pls_renderer" + libSuffix + extension),
                Path.Combine(libDirectory, libPrefix+ "rive_yoga" + libSuffix + extension),
                Path.Combine(libDirectory, libPrefix+ "rive" + libSuffix + extension),
            });

            PublicDefinitions.Add("WITH_RIVE=1");
            PublicDefinitions.Add("WITH_RIVE_AUDIO=1");
            PublicDefinitions.Add("EXTERNAL_RIVE_AUDIO_ENGINE=1");
        }
    }
}
