// Copyright Rive, Inc. All rights reserved.

using System.IO;
using UnrealBuildTool;

public class Rive : ModuleRules
{
	public Rive(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        var EngineDir = Path.GetFullPath(Target.RelativeEnginePath);

        PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
		);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// required for FPostProcessMaterialInputs
				Path.Combine(EngineDir, "Source/Runtime/Renderer/Private"),
            }
		);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"InputCore",
				"Projects",
				"RHI",
				"RenderCore",
				"RiveCore",
				"RiveLibrary",
				"RiveRenderer", 
				"Engine",
				// ... add other public dependencies that you statically link with here ...
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ApplicationCore",
				"CoreUObject",
				"Slate",
				"SlateCore",
				"Projects",
				"RiveCore",
				"RiveRenderer",
				"RHI",
				"RenderCore",
				"Renderer",
				"RiveLibrary",
				"Slate",
				"SlateCore",
				"UMG"
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
					"LevelEditor",
					"UnrealEd",
					"ViewportInteraction",
					"AssetTools",
					"TextureEditor",
				}
			);
		}
	}
}
