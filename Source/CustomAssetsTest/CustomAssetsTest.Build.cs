// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CustomAssetsTest : ModuleRules
{
	public CustomAssetsTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { 
            "Core", 
            "CoreUObject", 
            "Engine", 
            "InputCore", 
            "NavigationSystem", 
            "AIModule", 
            "Niagara", 
            "EnhancedInput",
            "AssetRegistry",
            "Json",
            "JsonUtilities"
        });
        
        if (Target.bBuildEditor)
        {
            PublicDependencyModuleNames.AddRange(new string[] {
                "UnrealEd",
                "Slate",
                "SlateCore",
                "EditorWidgets",
                "DesktopPlatform",
                "PropertyEditor",
                "LevelEditor",
                "EditorFramework",
                "ToolMenus",
                "ApplicationCore"
            });
        }
        
        // Add include paths for our custom asset system
        PublicIncludePaths.AddRange(new string[] {
            "CustomAssetsTest/Public"
        });
        
        PrivateIncludePaths.AddRange(new string[] {
            "CustomAssetsTest/Private"
        });
    }
}
