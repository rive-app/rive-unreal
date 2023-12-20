// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RiveEditor : ModuleRules
{
    public RiveEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", 
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "AssetDefinition",
                "CoreUObject",
                "Engine",
                "Rive",
                "Slate",
                "SlateCore",
                "UnrealEd",
                "PropertyEditor",
            }
        );
    }
}