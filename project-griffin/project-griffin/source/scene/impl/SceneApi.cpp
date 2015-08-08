#include <api/SceneApi.h>
#include <scene/Scene.h>
#include <entity/EntityManager.h>
#include <SDL_log.h>

griffin::scene::SceneManagerPtr g_sceneMgrPtr = nullptr;

void griffin::scene::setSceneManagerPtr(const griffin::scene::SceneManagerPtr& sceneMgrPtr)
{
	g_sceneMgrPtr = sceneMgrPtr;
}

namespace griffin {

	Id_T createEmptySceneNode(Id_T sceneId, Id_T parentEntityId)
	{
		auto& s = g_sceneMgrPtr->getScene(sceneId);
		auto entityId = s.entityManager->createEntity();

		auto sceneNodeId = s.sceneGraph->addToSceneEntity(entityId, {}, {}, parentEntityId);
		if (sceneNodeId != NullId_T) {
			return entityId;
		}
		return NullId_T;
	}


	Id_T createCamera(Id_T sceneId, Id_T parentEntityId,
					  scene::CameraParameters& cameraParams, const char name[32])
	{
		auto entityId = createEmptySceneNode(sceneId, parentEntityId);

		if (entityId != NullId_T) {
			auto& s = g_sceneMgrPtr->getScene(sceneId);

			scene::CameraInstanceContainer ci{};
			bool makePrimary = (s.cameras.size() == 0); // if this the first camera in the scene, make it primary
			ci.cameraId = s.createCamera(cameraParams, makePrimary);
			strcpy_s(ci.name, 32, name);

			auto camNodeId = s.entityManager->addComponentToEntity(std::move(ci), entityId);
			if (camNodeId != NullId_T) {
				return entityId;
			}
		}

		return NullId_T;
	}
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


	// Entity/Component functions

	uint64_t griffin_scene_createComponentStore(uint64_t scene, uint16_t typeId,
												uint32_t componentSize, size_t reserve)
	{
		SceneId sceneId;
		sceneId.value = scene;

		auto& s = g_sceneMgrPtr->getScene(sceneId);
		return s.entityManager->createScriptComponentStore(typeId, componentSize, reserve);
	}


	void* griffin_scene_getComponentData(uint64_t scene, uint64_t component)
	{
		SceneId sceneId;
		sceneId.value = scene;
		ComponentId cmpId;
		cmpId.value = component;

		auto& s = g_sceneMgrPtr->getScene(sceneId);
		return s.entityManager->getScriptComponentData(cmpId);
	}


	uint64_t griffin_scene_addComponentToEntity(uint64_t scene, uint16_t typeId, uint64_t entity)
	{
		SceneId sceneId;
		sceneId.value = scene;
		EntityId entityId;
		entityId.value = entity;

		auto& s = g_sceneMgrPtr->getScene(sceneId);
		return s.entityManager->addScriptComponentToEntity(typeId, entityId).value;
	}


	// Scene Node functions

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
										griffin_CameraParameters* cameraParams,
										const char name[32])
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
				bool makePrimary = (s.cameras.size() == 0); // if this the first camera in the scene, make it primary
				ci.cameraId = s.createCamera(cp, makePrimary);
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


	// Position, Orientation, Translation, Rotation functions

	scene::SceneNode* getSceneNode(uint64_t scene, uint64_t entity)
	{
		SceneId sceneId;
		sceneId.value = scene;
		EntityId entityId;
		entityId.value = entity;

		try {
			auto& s = g_sceneMgrPtr->getScene(sceneId);
			auto sceneNodeId = s.entityManager->getEntityComponentId(entityId, scene::SceneNode::componentType);
			if (sceneNodeId != NullId_T) {
				auto& store = s.entityManager->getComponentStore<scene::SceneNode>();
				return &store.getComponent(sceneNodeId);
			}
		}
		catch (std::exception ex) {
			SDL_Log("griffin_scene_getSceneNode: %s", ex.what());
		}
		return nullptr;
	}


	void griffin_scene_setRelativePosition(uint64_t scene, uint64_t entity, griffin_dvec3* pos)
	{
		auto sceneNode = getSceneNode(scene, entity);

		if (sceneNode != nullptr) {
			sceneNode->translationLocal = *reinterpret_cast<glm::dvec3*>(pos);
			sceneNode->positionDirty = 1;
		}
	}


	griffin_dvec3* griffin_scene_translate(uint64_t scene, uint64_t entity, griffin_dvec3* translation)
	{
		auto sceneNode = getSceneNode(scene, entity);

		if (sceneNode != nullptr) {
			auto& node = *sceneNode;
			node.translationLocal += *reinterpret_cast<glm::dvec3*>(translation);
			node.positionDirty = 1;
			return reinterpret_cast<griffin_dvec3*>(&node.translationLocal);
		}
		return nullptr;
	}

#ifdef __cplusplus
}
#endif