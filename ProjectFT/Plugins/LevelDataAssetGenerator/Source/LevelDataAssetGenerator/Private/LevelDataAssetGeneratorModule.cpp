#include "LevelDataAssetGeneratorLog.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogLevelDataAssetGenerator);

class FLevelDataAssetGeneratorModule : public IModuleInterface
{
};

IMPLEMENT_MODULE(FLevelDataAssetGeneratorModule, LevelDataAssetGenerator)
