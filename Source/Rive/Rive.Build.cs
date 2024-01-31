// Copyright Rive, Inc. All rights reserved.

using UnrealBuildTool;

public class Rive : ModuleRules
{
	public Rive(ReadOnlyTargetRules Target) : base(Target)
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
				"CoreUObject",
				"InputCore",
                "Projects",
                "RHICore",
                "RHI",
				"RenderCore",
                "RiveLibrary",
                "RiveRenderer",
				// ... add other public dependencies that you statically link with here ...
			}
            );
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ApplicationCore",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Projects",
				"RiveRenderer",
				"RHI",
				"RenderCore",
				"Renderer",
				"RiveLibrary",
				"Slate",
				"SlateCore",
				"UMG",
            }
			);
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
		
		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"EditorFramework",
					"LevelEditor",
					"UnrealEd",
					"ViewportInteraction",
				}
			);
		}
	}
}
