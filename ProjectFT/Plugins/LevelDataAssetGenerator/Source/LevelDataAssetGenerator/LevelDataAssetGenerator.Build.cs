using UnrealBuildTool;

public class LevelDataAssetGenerator : ModuleRules
{
	public LevelDataAssetGenerator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"DeveloperSettings",
			"Engine"
		});
	}
}
