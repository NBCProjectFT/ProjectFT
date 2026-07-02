using UnrealBuildTool;

public class ProjectFTEditor : ModuleRules
{
	public ProjectFTEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GoogleSheetLoader",
			"ProjectFT"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"AssetRegistry",
			"EditorScriptingUtilities",
			"RenderCore",
			"UnrealEd"
		});
	}
}
