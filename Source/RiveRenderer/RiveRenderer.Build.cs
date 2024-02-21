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
				"RiveCore"
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
                "Projects",
                "RHI",
				"RenderCore",
				"Renderer",
				"RiveCore",
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
