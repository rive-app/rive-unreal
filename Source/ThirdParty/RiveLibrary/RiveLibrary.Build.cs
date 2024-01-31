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

        string RootDir = ModuleDirectory;

        string IncludePath = Path.Combine(RootDir, "Includes");

        PublicSystemIncludePaths.Add(IncludePath);

        // NOTE: Incase if needed, otherwise feel free to remove it.
        bool bDebug = (Target.Configuration == UnrealTargetConfiguration.Debug && Target.bDebugBuildsActuallyUseDebugCRT);

        string LibPostfix = bDebug ? "_d" : "";

        if (Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
        {
            AddEngineThirdPartyPrivateStaticDependencies(Target, "DX9");

            AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11");

            // AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");

            PublicSystemLibraries.Add("d3dcompiler.lib");

            string LibDirectory = Path.Combine(RootDir, "Libraries", "Win64");

            string RiveLibPng = "rive_libpng" + LibPostfix + ".lib";

            string RiveSheenBidiStaticLibName = "rive_sheenbidi" + LibPostfix + ".lib";

            string RiveHarfBuzzStaticLibName = "rive_harfbuzz" + LibPostfix + ".lib";

            string RiveStaticLibName = "rive" + LibPostfix + ".lib";

            string RiveDecodersStaticLibName = "rive_decoders" + LibPostfix + ".lib";

            string RivePlsLibName = "rive_pls_renderer" + LibPostfix + ".lib";

            PublicAdditionalLibraries.AddRange(new string[]
                {
                    Path.Combine(LibDirectory, RiveSheenBidiStaticLibName),
                    Path.Combine(LibDirectory, RiveHarfBuzzStaticLibName),
                    Path.Combine(LibDirectory, RiveStaticLibName),
                    Path.Combine(LibDirectory, RiveDecodersStaticLibName),
                    Path.Combine(LibDirectory, RivePlsLibName),
                    Path.Combine(LibDirectory, RiveLibPng)
                }
            );

        bIsPlatformAdded = true;
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            string LibDirectory = Path.Combine(RootDir, "Libraries", "Mac");

            // NOTE : Link MacOS Libraries

            bIsPlatformAdded = true;
        }
        else if (Target.IsInPlatformGroup(UnrealPlatformGroup.Unix))
        {
            string LibDirectory = Path.Combine(RootDir, "Libraries", "Unix");

            // NOTE : Link Unix Libraries

            bIsPlatformAdded = true;
        }

        PublicDefinitions.Add("WITH_RIVE=" + (bIsPlatformAdded ? "1" : "0"));
    }
}
