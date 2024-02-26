// Copyright Rive, Inc. All rights reserved.

using System.IO;
using EpicGames.Core;
using UnrealBuildTool;

public class RiveCore : ModuleRules
{
	
	public RiveCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory, "Public")
				// ... other include paths
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory, "Private")
				// ... other include paths
			}
		);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"RHICore",
				"RHI",
				"RenderCore"
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
                "Projects",
				"RiveLibrary",
				"RiveRenderer",
			}
		);
	}
}
