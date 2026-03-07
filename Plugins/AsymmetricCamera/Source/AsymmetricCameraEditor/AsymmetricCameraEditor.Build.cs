// Copyright Epic Games, Inc. All Rights Reserved.
// Compatible with Unreal Engine 5.4+

using UnrealBuildTool;

public class AsymmetricCameraEditor : ModuleRules
{
	public AsymmetricCameraEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"AsymmetricCamera"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"ComponentVisualizers"
			}
		);
	}
}
