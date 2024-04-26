// Copyright Rive, Inc. All rights reserved.

using System.IO;

namespace UnrealBuildTool.Rules
{
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
					"GeometricObjects",
					"RHI",
					"RenderCore",
					"AudioMixer"
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
}
