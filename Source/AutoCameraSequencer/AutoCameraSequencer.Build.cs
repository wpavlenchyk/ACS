// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AutoCameraSequencer : ModuleRules
{
	public AutoCameraSequencer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]{});

		PrivateDependencyModuleNames.AddRange(new string[]
			{
				"Core",
				"CoreUObject",
				"CinematicCamera",
				"LevelSequence",
				"MovieScene",
				"MovieSceneTracks",
				"MovieSceneTools",
				"MovieSceneCapture",
				"AVIWriter",
				"SequencerScriptingEditor",
			}
		);

	}
}