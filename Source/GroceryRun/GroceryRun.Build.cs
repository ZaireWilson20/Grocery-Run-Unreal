// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GroceryRun : ModuleRules
{
	public GroceryRun(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] { "AIModule", "NavigationSystem", "Noesis", "NoesisRuntime"});
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
