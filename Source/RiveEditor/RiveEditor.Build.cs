// Copyright Rive, Inc. All rights reserved.

using System.IO;
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
                "CoreUObject",
                "ContentBrowser",
                "EditorStyle",
                "Engine",
                "PropertyEditor",
                "RHI",
                "Rive",
                "RiveCore",
                "Slate",
                "SlateCore",
                "UnrealEd",
                "ToolMenus",
                "UMG",
                "UMGEditor", 
                "SettingsEditor",
            }
        );

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PrivateDependencyModuleNames.Add("WindowsTargetPlatform");
        }

        // Adding the path needed to include the private file WidgetTemplateClass.h
        string EnginePath = Path.GetFullPath(Target.RelativeEnginePath);
        string SourcePath = Path.Combine(EnginePath, "Source");
        PrivateIncludePaths.Add(Path.Combine(SourcePath, "Editor", "UMGEditor", "Private"));
    }
}