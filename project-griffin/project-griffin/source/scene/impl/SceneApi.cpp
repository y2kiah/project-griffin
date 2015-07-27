#include <api/SceneApi.h>
#include <scene/Scene.h>
#include <SDL_log.h>

griffin::scene::SceneManagerPtr g_sceneMgrPtr = nullptr;

void griffin::scene::setSceneManagerPtr(const SceneManagerPtr& sceneMgrPtr)
{
	g_sceneMgrPtr = sceneMgrPtr;
}


#ifdef __cplusplus
extern "C" {
#endif

	using namespace griffin;
	using namespace griffin::scene;

	
	uint64_t griffin_scene_createScene(const char name[32], bool makeActive)
	{
		return g_sceneMgrPtr->createScene(name, makeActive).value;
	}

#ifdef __cplusplus
}
#endif