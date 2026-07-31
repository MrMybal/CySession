// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CySession : ModuleRules
{
	public CySession(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"DeveloperSettings",
				"Engine",
				"OnlineSubsystem"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"OnlineSubsystemUtils"
			}
		);
	}
}
