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
        CppStandard = CppStandardVersion.Cpp20;
        
        string rootDir = ModuleDirectory;
        string includePath = Path.Combine(rootDir, "Includes");

        PublicSystemIncludePaths.Add(includePath);
        AddEngineThirdPartyPrivateStaticDependencies(Target, "zlib");
        // NOTE: Incase if needed, otherwise feel free to remove it.
        // Unreal "Debug" configs don't set bDebugBuildsActuallyUseDebugCRT, so we never actually use the debug libs
        // https://dev.epicgames.com/documentation/en-us/unreal-engine/build-configuration-for-unreal-engine?application_version=5.1
        bool bDebug = (Target.Configuration == UnrealTargetConfiguration.Debug && Target.bDebugBuildsActuallyUseDebugCRT);

        string libSuffix = bDebug ? "_d" : "";

        var details = new List<NativeLibraryDetails>();

        if (Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
        {
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
        else if (Target.IsInPlatformGroup(UnrealPlatformGroup.Linux))
        {
            // The Linux (gmake) build prefixes every archive with "lib", and
            // build-rive.py then prepends "rive_", so the core libs are named
            // "rive_librive*.a" and the vendored libs "rive_liblib*.a". The "rive_lib"
            // prefix maps the base names used in the loop below to those filenames.
            details.Add(new NativeLibraryDetails(".a", libSuffix, "rive_lib", Path.Combine(rootDir, "Libraries", "Linux")));
        }

		foreach (var detail in details)
		{
	        if (Target.IsInPlatformGroup(UnrealPlatformGroup.Apple) || Target.IsInPlatformGroup(UnrealPlatformGroup.Android) || Target.IsInPlatformGroup(UnrealPlatformGroup.Linux))
	        {
	            // Apple/Android/Linux keep the vendored libs' "lib" name (e.g. libwebp);
	            // the per-platform prefix in the detail turns it into the real filename.
	            PublicAdditionalLibraries.AddRange(new string[]
	           {
	                    detail.GetLibPath("libwebp"),
	                    detail.GetLibPath("libpng"),
	                    detail.GetLibPath("libjpeg"),
                        detail.GetLibPath("miniaudio"),
	           });
	        }
	        else
	        {
	            PublicAdditionalLibraries.AddRange(new string[]
	           {
	                    detail.GetLibPath("rive_libwebp"),
	                    detail.GetLibPath("rive_libpng"),
	                    detail.GetLibPath("rive_libjpeg"),
                        detail.GetLibPath("rive_miniaudio"),
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

            // You don't have to build with scripting enabled.
            // This checks for the library to be in the folder
            // before attempting to add it.
            // On Linux build-rive.py does not add the "rive_" prefix to luau, so it is
            // "libluau_vm.a" rather than matching the "rive_lib" naming used above.
            var luauPath = Target.IsInPlatformGroup(UnrealPlatformGroup.Linux)
                ? Path.Combine(detail.LibDirectory, "libluau_vm" + libSuffix + ".a")
                : detail.GetLibPath("luau_vm");
            if (File.Exists(luauPath))
            {
                PublicAdditionalLibraries.Add(luauPath);
            }
		}

		PublicDefinitions.Add("RIVE_UNREAL");
		PublicDefinitions.Add("WITH_RIVE=1");
		PublicDefinitions.Add("RIVE_CANVAS=1");
		PublicDefinitions.Add("ORE_BACKEND_RHI=1");
		PublicDefinitions.Add("TRACK_RIVE_SHADER_ID");
        PublicDefinitions.Add("RIVE_WITH_UNREAL=1");
        PublicDefinitions.Add("RIVE_ORE=1");
        PublicDefinitions.Add("WITH_RIVE_AUDIO=1");
        PublicDefinitions.Add("EXTERNAL_RIVE_AUDIO_ENGINE=1");
        PublicDefinitions.Add("WITH_RIVE_SCRIPTING=1");

        // If we are linking against GMs, define WITH_RIVE_TOOLS
        // This has to be done here because it affects FlushDescriptor
        var GMPluginsFldr = Path.Combine(PluginDirectory, "../", "GM");
        if (Directory.Exists(GMPluginsFldr))
        {
            PublicDefinitions.Add("WITH_RIVE_TOOLS=1");
        }
    }
}
