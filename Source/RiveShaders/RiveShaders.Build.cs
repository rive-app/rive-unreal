// Copyright Rive, Inc. All rights reserved.
using UnrealBuildTool;

public class RiveShaders : ModuleRules
{
	public RiveShaders(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", "RiveLibrary"
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"DeveloperSettings",
				"Engine",
				"RHI",
				"RenderCore",
				"Renderer",
				"Projects"
			}
		);

#if UE_5_0_OR_LATER
		PublicDependencyModuleNames.Add("RHICore");
#else
#endif
	}
}
