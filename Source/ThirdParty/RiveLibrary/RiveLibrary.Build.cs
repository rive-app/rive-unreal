// Copyright Rive, Inc. All rights reserved.

using EpicGames.Core;
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

        if (Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
        {
            string extension = "lib";
            AddEngineThirdPartyPrivateStaticDependencies(Target, "DX9");
            AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11");
            // AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");

            PublicSystemLibraries.Add("d3dcompiler.lib");

            string libDirectory = Path.Combine(rootDir, "Libraries", "Win64");
            
            PublicAdditionalLibraries.AddRange(new string[]
            {
                Path.Combine(libDirectory, $"rive_sheenbidi{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"rive_harfbuzz{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"rive_libpng{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"rive_libjpeg{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"rive_decoders{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"rive_pls_renderer{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"rive_yoga{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"rive{libSuffix}.{extension}"),
            });

            bIsPlatformAdded = true;
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            string extension = "a";
            string libDirectory = Path.Combine(rootDir, "Libraries", "Mac");
            if (Target.Architecture == UnrealArch.Arm64)
            {
                libDirectory = Path.Combine(libDirectory, "Mac");
                PublicDefinitions.Add("WITH_RIVE_MAC_ARM64 = 1");
            }
            else
            {
                libDirectory = Path.Combine(libDirectory, "Intel");
                PublicDefinitions.Add("WITH_RIVE_MAC_INTEL = 1");
            }
            
            PublicAdditionalLibraries.AddRange(new string[]
            {
                Path.Combine(libDirectory, $"librive_sheenbidi{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"librive_harfbuzz{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"liblibpng{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"liblibjpeg{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"librive_decoders{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"librive_pls_renderer{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"librive_yoga{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"librive{libSuffix}.{extension}"),
            });
            
            bIsPlatformAdded = true;
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            string libDirectory = Path.Combine(rootDir, "Libraries", "IOS");
            string extension = (Target.Architecture == UnrealArch.IOSSimulator) ? "sim.a" : "a";

            PublicAdditionalLibraries.AddRange(new string[]
            {
                Path.Combine(libDirectory, $"librive_sheenbidi{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"librive_harfbuzz{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"liblibpng{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"liblibjpeg{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"librive_decoders{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"librive_pls_renderer{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"librive_yoga{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"librive{libSuffix}.{extension}"),
            });

            bIsPlatformAdded = true;
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            PrivateDependencyModuleNames.Add("OpenGLDrv");
            AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenGL");
            
            string libDirectory = Path.Combine(rootDir, "Libraries", "Android");
            string extension = "a";
            
            PublicRuntimeLibraryPaths.Add(libDirectory);
            PublicAdditionalLibraries.AddRange(new string[]
            {
                Path.Combine(libDirectory, $"librive_sheenbidi{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"librive_harfbuzz{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"liblibpng{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"liblibjpeg{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"librive_decoders{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"librive_pls_renderer{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"librive_yoga{libSuffix}.{extension}"),
                Path.Combine(libDirectory, $"librive{libSuffix}.{extension}"),
            });
            
            PrecompileForTargets = PrecompileTargetsType.None;
            bIsPlatformAdded = true;
            
            string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
            AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(PluginPath, "RiveLibrary_APL.xml"));
        }
        else if (Target.IsInPlatformGroup(UnrealPlatformGroup.Unix))
        {
            string libDirectory = Path.Combine(rootDir, "Libraries", "Unix");
            
            // NOTE : Link Unix Libraries

            bIsPlatformAdded = true;
        }

        if (bIsPlatformAdded)
        {
            PublicDefinitions.Add("WITH_RIVE=1");
            PublicDefinitions.Add("WITH_RIVE_AUDIO=1");
            PublicDefinitions.Add("EXTERNAL_RIVE_AUDIO_ENGINE=1");
        }
    }
}
