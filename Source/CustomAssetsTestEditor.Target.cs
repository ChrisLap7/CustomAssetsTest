// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class CustomAssetsTestEditorTarget : TargetRules
{
	public CustomAssetsTestEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		
		// IMPORTANT: We are only adding the main module, not a separate editor module
		ExtraModuleNames.Add("CustomAssetsTest");
		
		// Explicitly clear search locations for any CustomAssetsTestEditor module
		// This ensures Visual Studio doesn't try to build a non-existent module
		bUseUnityBuild = true;
		bUsePCHFiles = true;
	}
}
