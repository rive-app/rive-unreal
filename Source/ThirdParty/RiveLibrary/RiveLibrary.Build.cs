// Copyright Rive, Inc. All rights reserved.
#if UE_5_0_OR_LATER
using EpicGames.Core;
#else
using Tools.DotNETCommon;
#endif
using System.Collections.Generic;
using System.IO;
using UnrealBuildTool;

public class RiveLibrary : ModuleRules
{
    // Added for multi Arch build support
    public struct NativeLibraryDetails
    {
        public string Extension;
        public string LibSuffix;
        public string LibPrefix;
        public string LibDirectory;

		public NativeLibraryDetails(string extension, string libSuffix, string libPrefix, string libDirectory)
		{
			Extension = extension;
			LibSuffix = libSuffix;
			LibPrefix = libPrefix;
			LibDirectory = libDirectory;
		}

        public string GetLibPath(string baseName)
        {
            return Path.Combine(LibDirectory, LibPrefix + baseName + LibSuffix + Extension);
        }
    }
    public RiveLibrary(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;
        CppStandard = CppStandardVersion.Cpp17;
		
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

        var details = new List<NativeLibraryDetails>();

        if (Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
        {
            AddEngineThirdPartyPrivateStaticDependencies(Target, "DX9");
            AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11");
            // AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");

            PublicSystemLibraries.Add("d3dcompiler.lib");
            
            details.Add(new NativeLibraryDetails(".lib",libSuffix,"",Path.Combine(rootDir, "Libraries", "Win64")));
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
#if UE_5_4_OR_LATER
			if (Target.Architectures.Contains(UnrealArch.Arm64))
#elif UE_5_0_OR_LATER
            if (Target.Architecture == UnrealArch.Arm64)
#else
            if (Target.Architecture.Contains("arm64"))
#endif
            {
                string libDirectory = Path.Combine(rootDir, "Libraries", "Mac", "Mac");
                PublicDefinitions.Add("WITH_RIVE_MAC_ARM64 = 1");
				details.Add(new NativeLibraryDetails(".a", libSuffix,"lib", libDirectory));
            }
#if UE_5_4_OR_LATER
			if(Target.Architectures.Contains(UnrealArch.X64))
#else 
			else
#endif
            {
                string libDirectory = Path.Combine(rootDir, "Libraries", "Mac", "Intel");
                PublicDefinitions.Add("WITH_RIVE_MAC_INTEL = 1");
				details.Add(new NativeLibraryDetails(".a", libSuffix,"lib", libDirectory));
            }
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
			PublicFrameworks.Add("CoreText");

#if UE_5_4_OR_LATER
            if (Target.Architectures.Contains( UnrealArch.IOSSimulator))
#elif UE_5_0_OR_LATER
            if (Target.Architecture == UnrealArch.IOSSimulator)
#else
            if (Target.Architecture.Contains("sim"))
#endif
			{
				details.Add(new NativeLibraryDetails(".sim.a", libSuffix,"lib", Path.Combine(rootDir, "Libraries", "IOS")));
			}
#if UE_5_4_OR_LATER
            if (!Target.Architectures.Contains( UnrealArch.IOSSimulator))
#elif UE_5_0_OR_LATER
            if (!(Target.Architecture == UnrealArch.IOSSimulator))
#else
            if (!Target.Architecture.Contains("sim"))
#endif
			{
				details.Add(new NativeLibraryDetails(".a", libSuffix, "lib", Path.Combine(rootDir, "Libraries", "IOS")));
			}
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            PrivateDependencyModuleNames.Add("OpenGLDrv");
            AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenGL");

            string libDirectory = Path.Combine(rootDir, "Libraries", "Android");
            PublicRuntimeLibraryPaths.Add(libDirectory);
			details.Add(new NativeLibraryDetails(".a",libSuffix,"lib", libDirectory));

            PrecompileForTargets = PrecompileTargetsType.None;
            string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
            AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(PluginPath, "RiveLibrary_APL.xml"));
			
        }
        else if (Target.IsInPlatformGroup(UnrealPlatformGroup.Unix))
        {
            //libDirectory = Path.Combine(rootDir, "Libraries", "Unix");
            //extension = ".a";
            // NOTE : Link Unix Libraries

        }

		foreach (var detail in details)
		{
	        if (Target.IsInPlatformGroup(UnrealPlatformGroup.Apple) || Target.IsInPlatformGroup(UnrealPlatformGroup.Android))
	        {
	            PublicAdditionalLibraries.AddRange(new string[]
	           {
	                    detail.GetLibPath("libwebp"),
	                    detail.GetLibPath("libpng"),
	                    detail.GetLibPath("libjpeg"),
	           });
	        }
	        else
	        {
	            PublicAdditionalLibraries.AddRange(new string[]
	           {
	                    detail.GetLibPath("rive_libwebp"),
	                    detail.GetLibPath("rive_libpng"),
	                    detail.GetLibPath("rive_libjpeg"),
	           });
	        }

	        PublicAdditionalLibraries.AddRange(new string[]
	        {
	            detail.GetLibPath("rive_sheenbidi"),
	            detail.GetLibPath("rive_harfbuzz"),
	            detail.GetLibPath("rive_decoders"),
	            detail.GetLibPath("rive_pls_renderer"),
	            detail.GetLibPath("rive_yoga"),
	            detail.GetLibPath("rive"),
	        });
		}

        PublicDefinitions.Add("WITH_RIVE=1");
        PublicDefinitions.Add("WITH_RIVE_AUDIO=1");
        PublicDefinitions.Add("EXTERNAL_RIVE_AUDIO_ENGINE=1");
        
    }
}
