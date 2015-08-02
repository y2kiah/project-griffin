#include <api/SceneApi.h>
#include <scene/Scene.h>
#include <entity/EntityManager.h>
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

	// do some sanity checks to make sure the API structs are the same size
	// the layout must also match but that isn't checked here
	static_assert(sizeof(griffin_CameraParameters) == sizeof(CameraParameters), "CameraParameters size out of sync");

	
	uint64_t griffin_scene_createScene(const char name[32], bool makeActive)
	{
		return g_sceneMgrPtr->createScene(name, makeActive).value;
	}


	uint64_t griffin_scene_createEmptySceneNode(uint64_t scene, uint64_t parentEntity)
	{
		SceneId sceneId;
		sceneId.value = scene;
		EntityId parentId;
		parentId.value = parentEntity;

		try {
			auto& s = g_sceneMgrPtr->getScene(sceneId);
			auto entityId = s.entityManager->createEntity();

			auto sceneNodeId = s.sceneGraph->addToSceneEntity(entityId, {}, {}, parentId);
			if (sceneNodeId != NullId_T) {
				return entityId.value;
			}
		}
		catch (std::exception ex) {
			SDL_Log("griffin_scene_createEmptySceneNode: %s", ex.what());
		}
		return 0;
	}

	
	uint64_t griffin_scene_createMeshInstance(uint64_t scene, uint64_t parentEntity, uint64_t mesh)
	{
		EntityId entityId{};
		entityId.value = griffin_scene_createEmptySceneNode(scene, parentEntity);

		if (entityId != NullId_T) {
			try {
				SceneId sceneId;
				sceneId.value = scene;
				auto& s = g_sceneMgrPtr->getScene(sceneId);

				scene::MeshInstanceContainer mi{};
				// TODO: request the meshId from resource system

				s.entityManager->addComponentToEntity<scene::MeshInstanceContainer>(std::move(mi), entityId);

				return entityId.value;
			}
			catch (std::exception ex) {
				SDL_Log("griffin_scene_createMeshInstance: %s", ex.what());
			}
		}
		return 0;
	}

	
	uint64_t griffin_scene_createCamera(uint64_t scene, uint64_t parentEntity,
										griffin_CameraParameters* cameraParams, const char name[32])
	{
		EntityId entityId{};
		entityId.value = griffin_scene_createEmptySceneNode(scene, parentEntity);

		if (entityId != NullId_T) {
			try {
				SceneId sceneId;
				sceneId.value = scene;
				auto& s = g_sceneMgrPtr->getScene(sceneId);

				CameraParameters cp{};
				griffin_CameraParameters& inCp = *cameraParams;
				cp.nearClipPlane	= inCp.nearClipPlane;
				cp.farClipPlane		= inCp.farClipPlane;
				cp.viewportWidth	= inCp.viewportWidth;
				cp.viewportHeight	= inCp.viewportHeight;
				cp.verticalFieldOfViewDegrees = inCp.verticalFieldOfViewDegrees;
				cp.cameraType		= inCp.cameraType;

				scene::CameraInstanceContainer ci{};
				ci.cameraId = s.createCamera(cp, false);
				strcpy_s(ci.name, 32, name);
				
				s.entityManager->addComponentToEntity<scene::CameraInstanceContainer>(std::move(ci), entityId);

				return entityId.value;
			}
			catch (std::exception ex) {
				SDL_Log("griffin_scene_createMeshInstance: %s", ex.what());
			}
		}

		return 0;
	}

	
	uint64_t griffin_scene_createLight(uint64_t scene, uint64_t parentEntity)
	{
		return 0;
	}

#ifdef __cplusplus
}
#endif