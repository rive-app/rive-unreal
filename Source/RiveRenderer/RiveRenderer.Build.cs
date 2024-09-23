// Copyright Rive, Inc. All rights reserved.

using System.IO;
using EpicGames.Core;
using UnrealBuildTool;

public class RiveRenderer : ModuleRules
{
	
	public RiveRenderer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
		);
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
		);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core"
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"DeveloperSettings",
				"UnrealEd",
				"Engine",
                "RHI",
				"RenderCore",
				"Renderer",
				"RiveLibrary",
			}
		);
		
		if (Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
		{
			PublicDependencyModuleNames.Add("D3D11RHI");
			PublicIncludePathModuleNames.AddAll("RHICore", "D3D11RHI"); // , "D3D12RHI");

			AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11");
			// AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");

			// Adding the path needed to include the private file D3D11RHIPrivate.h
			string enginePath = Path.GetFullPath(Target.RelativeEnginePath);
			string sourcePath = Path.Combine(enginePath, "Source", "Runtime");
			PrivateIncludePaths.Add(Path.Combine(sourcePath, "Windows", "D3D11RHI", "Private"));
			PublicIncludePaths.Add(Path.Combine(sourcePath, "Windows", "D3D11RHI", "Private"));

			// Copied from D3D11RHI.build.cs, needed to include the private file D3D11RHIPrivate.h
			AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelExtensionsFramework");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAftermath");
			if (Target.Version.MajorVersion == 5 && Target.Version.MinorVersion < 4)
			{
				AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelMetricsDiscovery");
			}
		}
		else if (Target.Platform.IsInGroup(UnrealPlatformGroup.Android))
		{
			PublicDependencyModuleNames.Add("OpenGLDrv");
			PublicDependencyModuleNames.Add("OpenGL");
			
			PublicIncludePathModuleNames.AddAll("RHICore", "OpenGLDrv");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenGL");
			
			// The below is needed to include private headers from OpenGLDrv, allowing us to call directly some OpenGL functions
			string enginePath = Path.GetFullPath(Target.RelativeEnginePath);
			string sourcePath = Path.Combine(enginePath, "Source", "Runtime", "OpenGLDrv", "Private");
			
			PrivateIncludePaths.Add(Path.Combine(sourcePath));
			PrivateIncludePaths.Add(Path.Combine(sourcePath, "Android"));
			PublicIncludePaths.Add(Path.Combine(sourcePath));
			PublicIncludePaths.Add(Path.Combine(sourcePath, "Android"));
		}
        else if (Target.Platform.IsInGroup(UnrealPlatformGroup.Apple))
        {
            PublicIncludePathModuleNames.AddAll("MetalRHI");
            
            PublicDependencyModuleNames.Add("MetalRHI");
        }
	}
}
