// Copyright 2025, Aquanox.

using System.IO;
using UnrealBuildTool;

public class EnhancedPalette : ModuleRules
{
    public bool bStrictIncludesCheck = false;

	public EnhancedPalette(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
        if (bStrictIncludesCheck)
        {
            bUseUnity = false;
            PCHUsage = PCHUsageMode.NoPCHs;
            // Enable additional checks used for Engine modules
            bTreatAsEngineModule = true;
        }

        // Include private headers of Placement Mode Module to include path
        PrivateIncludePaths.Add(Path.GetFullPath(Target.RelativeEnginePath) + "/Source/Editor/PlacementMode/Private");

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
		});


		PrivateDependencyModuleNames.AddRange(new string[] {
			"Engine",
			"Slate",
			"SlateCore",
			"UnrealEd",
			"PlacementMode",
			"DeveloperSettings",
			"EditorSubsystem",
			"EditorFramework",
			"LevelEditor",
			"InputCore",
			"DetailCustomizations",
			"WorkspaceMenuStructure",
			"WidgetRegistration",
			"ToolWidgets",
			"PropertyEditor",
			"BlueprintGraph"
		} );

		if (Target.Version.MajorVersion == 5 && Target.Version.MinorVersion < 5)
		{
			PrivateDependencyModuleNames.Add("StructUtils");
		}
	}
}
