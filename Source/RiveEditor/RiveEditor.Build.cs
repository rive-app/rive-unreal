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
                "AssetTools",
                "AssetDefinition",
                "CoreUObject",
                "ContentBrowser",
                "Engine",
                "PropertyEditor",
                "Rive",
                "RiveLibrary",
                "Slate",
                "SlateCore",
                "UnrealEd",
                "ToolMenus",
                "UMG",
                "UMGEditor", 
                "SettingsEditor",
                "EditorStyle",
                "UnrealEd",
                "DeveloperSettings",
                "RiveRenderer"
            }
        );

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PrivateDependencyModuleNames.Add("WindowsTargetPlatform");
        }
    }
}