// Copyright Rive, Inc. All rights reserved.

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
                "ContentBrowser",
                "Engine",
                "PropertyEditor",
                "Rive",
                "Slate",
                "SlateCore",
                "UnrealEd",
                "ToolMenus",
                "UMG",
                "UMGEditor",
            }
        );
    }
}