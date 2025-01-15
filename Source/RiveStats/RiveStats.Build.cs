// Copyright Rive, Inc. All rights reserved.
using UnrealBuildTool;

public class RiveStats : ModuleRules
{
	public RiveStats(ReadOnlyTargetRules Target) : base(Target)
	{
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
				"Engine",
				"Projects"
			}
		);
	}
}
