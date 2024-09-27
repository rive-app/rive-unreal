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
				"AudioMixer",
				"Core",
				"CoreUObject",
				"InputCore",
				"Projects",
				"RHI",
				"RenderCore",
				"RiveLibrary",
				"RiveRenderer",
				"Engine"
				// ... add other public dependencies that you statically link with here ...
			}
		);
		
#if UE_5_0_OR_LATER
		PublicDependencyModuleNames.Add("RHICore");
#else
#endif


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ApplicationCore",
				"Core",
				"CoreUObject",
				"Engine",
				"Projects",
				"RHI",
				"RenderCore",
				"Renderer",
				"RiveLibrary",
				"RiveRenderer",
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
#if UE_5_0_OR_LATER
					"EditorFramework",
#endif
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
