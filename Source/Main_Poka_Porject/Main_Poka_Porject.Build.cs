// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Main_Poka_Porject : ModuleRules
{
	public Main_Poka_Porject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AIModule", "NavigationSystem", "UMG", "Slate", "SlateCore" });
	}
}
