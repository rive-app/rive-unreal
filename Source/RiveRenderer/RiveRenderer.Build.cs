// Copyright Rive, Inc. All rights reserved.

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
	}
}