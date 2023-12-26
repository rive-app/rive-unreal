// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RiveCore : ModuleRules
{
	public RiveCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
                "CoreUObject",
                "RHI",
                "RiveLibrary",
            });

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Engine",
				"Slate",
				"SlateCore",
                "RiveLibrary",
            });


        if (Target.bBuildEditor == true)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[] {
                    "UnrealEd"
                });
        }
    }
}
