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
            if (Target.Architecture == UnrealArch.Arm64)
            {
                LibDirectory = Path.Combine(LibDirectory, "Mac");
                PublicDefinitions.Add("WITH_RIVE_MAC_ARM64 = 1");
            }
            else
            {
                LibDirectory = Path.Combine(LibDirectory, "Intel");
                PublicDefinitions.Add("WITH_RIVE_MAC_INTEL = 1");
            }
            string RiveSheenBidiStaticLibName = "librive_sheenbidi" + LibPostfix + ".a"; ;
            string RiveHarfBuzzStaticLibName = "librive_harfbuzz" + LibPostfix + ".a"; ;
            string RiveStaticLibName = "librive" + LibPostfix + ".a"; ;
            string RiveDecodersStaticLibName = "librive_decoders" + LibPostfix + ".a"; ;
            string RivePlsLibName = "librive_pls_renderer" + LibPostfix + ".a"; ;
            string RivePngLibName = "liblibpng" + LibPostfix + ".a";

            PublicAdditionalLibraries.AddRange(new string[] { Path.Combine(LibDirectory, RiveSheenBidiStaticLibName)
                , Path.Combine(LibDirectory, RiveHarfBuzzStaticLibName)
                , Path.Combine(LibDirectory, RiveStaticLibName)
                , Path.Combine(LibDirectory, RiveDecodersStaticLibName)
                , Path.Combine(LibDirectory, RivePlsLibName)
                , Path.Combine(LibDirectory, RivePngLibName) });

            bIsPlatformAdded = true;
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            string LibDirectory = Path.Combine(RootDir, "Libraries", "IOS");
            string LibExt = (Target.Architecture == UnrealArch.IOSSimulator) ? ".sim.a" : ".a";
            string RiveSheenBidiStaticLibName = "librive_sheenbidi" + LibPostfix + LibExt; ;
            string RiveHarfBuzzStaticLibName = "librive_harfbuzz" + LibPostfix + LibExt; ;
            string RiveStaticLibName = "librive" + LibPostfix + LibExt; ;
            string RiveDecodersStaticLibName = "librive_decoders" + LibPostfix + LibExt; ;
            string RivePlsLibName = "librive_pls_renderer" + LibPostfix + LibExt; ;
            string RivePngLibName = "liblibpng" + LibPostfix + LibExt;

            PublicAdditionalLibraries.AddRange(new string[] { Path.Combine(LibDirectory, RiveSheenBidiStaticLibName)
                , Path.Combine(LibDirectory, RiveHarfBuzzStaticLibName)
                , Path.Combine(LibDirectory, RiveStaticLibName)
                , Path.Combine(LibDirectory, RiveDecodersStaticLibName)
                , Path.Combine(LibDirectory, RivePlsLibName)
                , Path.Combine(LibDirectory, RivePngLibName) });

            bIsPlatformAdded = true;
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            PrivateDependencyModuleNames.Add("OpenGLDrv");
            AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenGL");
            
            string LibDirectory = Path.Combine(RootDir, "Libraries", "Android");
            string RiveSheenBidiStaticLibName = "librive_sheenbidi" + LibPostfix + ".a";
            string RiveHarfBuzzStaticLibName = "librive_harfbuzz" + LibPostfix + ".a";
            string RiveStaticLibName = "librive" + LibPostfix + ".a"; ;
            string RiveDecodersStaticLibName = "librive_decoders" + LibPostfix + ".a";
            string RivePlsLibName = "librive_pls_renderer" + LibPostfix + ".a";
            string RivePngLibName = "liblibpng" + LibPostfix + ".a";

            PublicRuntimeLibraryPaths.Add(LibDirectory);
            PublicAdditionalLibraries.AddRange(new string[] 
                { 
                    Path.Combine(LibDirectory, RiveSheenBidiStaticLibName)
                    , Path.Combine(LibDirectory, RiveHarfBuzzStaticLibName)
                    , Path.Combine(LibDirectory, RiveStaticLibName)
                    , Path.Combine(LibDirectory, RiveDecodersStaticLibName)
                    , Path.Combine(LibDirectory, RivePlsLibName)
                    , Path.Combine(LibDirectory, RivePngLibName)
                }
            );
            
            PrecompileForTargets = PrecompileTargetsType.None;
            bIsPlatformAdded = true;
            
            string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
            AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(PluginPath, "RiveLibrary_APL.xml"));
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
