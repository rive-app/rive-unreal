// Copyright 2024, 2025 Rive, Inc. All rights reserved.

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
#if UE_5_0_OR_LATER
                "AssetDefinition",
#endif
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
                "DeveloperSettings",
                "RiveRenderer",
                "RHI",
                "RenderCore",
                "BlueprintGraph", 
                "EditorScriptingUtilities"
            }
        );

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
#if UE_5_5_OR_LATER
            PrivateDependencyModuleNames.Add("WindowsTargetPlatformSettings");
#else
            PrivateDependencyModuleNames.Add("WindowsTargetPlatform");
#endif
        }
    }
}