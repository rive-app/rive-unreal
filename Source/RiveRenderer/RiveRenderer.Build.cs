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
				"Core",
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"RHI",
				"RenderCore",
				"Renderer",
				"RiveLibrary",
			}
		);
		
		if (Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
		{
			PublicIncludePathModuleNames.AddAll("D3D11RHI"); // , "D3D12RHI");

			AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11");
			
			// AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
		}
		else if (Target.Platform.IsInGroup(UnrealPlatformGroup.Android))
		{
			PublicDependencyModuleNames.Add("OpenGLDrv");
			PublicDependencyModuleNames.Add("OpenGL");
			
			PublicIncludePathModuleNames.AddAll("RHICore", "OpenGLDrv");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenGL");
			
			string engine_path = Path.GetFullPath(Target.RelativeEnginePath);
			string srcrt_path = Path.Combine(engine_path, "Source", "Runtime/");
			
			PrivateIncludePaths.Add(Path.Combine(srcrt_path, "OpenGLDrv", "Private"));
			PrivateIncludePaths.Add(Path.Combine(srcrt_path, "OpenGLDrv", "Private", "Android"));
			PublicIncludePaths.Add(Path.Combine(srcrt_path, "OpenGLDrv", "Private"));
			PublicIncludePaths.Add(Path.Combine(srcrt_path, "OpenGLDrv", "Private", "Android"));
			// bUseRTTI = true;
		}
	}
}