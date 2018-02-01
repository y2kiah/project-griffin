#include <api/SceneApi.h>
#include <scene/Scene.h>
#include <entity/EntityManager.h>
#include <utility/Logger.h>
#include <game/positionalEffects/screenShaker/ScreenShakerComponents.h>

griffin::scene::SceneManagerPtr g_sceneMgrPtr = nullptr;

void griffin::scene::setSceneManagerPtr(const griffin::scene::SceneManagerPtr& sceneMgrPtr)
{
	g_sceneMgrPtr = sceneMgrPtr;
}

namespace griffin {

	EntityId createNewSceneNode(
		SceneId sceneId,
		bool movable,
		SceneNodeId parentNode)
	{
		auto& s = g_sceneMgrPtr->getScene(sceneId);
		auto entityId = s.entityManager->createEntity();

		auto sceneNodeId = s.sceneGraph->addToScene(entityId, {}, {}, parentNode);
		
		if (movable) {
			scene::MovementComponent mc{};
			mc.sceneNodeId = sceneNodeId;

			s.entityManager->addComponentToEntity(std::move(mc), entityId);
		}

		return entityId;
	}


	// TODO: pass in model resource id, position, orientation and scale
	EntityId createNewModelInstance(
		SceneId sceneId,
		bool movable,
		SceneNodeId parentNode)
	{
		auto& s = g_sceneMgrPtr->getScene(sceneId);

		auto entityId = createNewSceneNode(sceneId, movable, parentNode);
		auto sceneNodeId = s.entityManager->getEntityComponentId(entityId, scene::SceneNode::componentType);

		// add a ModelInstance component
		scene::ModelInstance mi{};
		mi.sceneNodeId = sceneNodeId;
		//mi.modelId = ?;

		s.entityManager->addComponentToEntity(std::move(mi), entityId);

		return entityId;
	}


	EntityId createNewCamera(
		SceneId sceneId,
		scene::CameraParameters& cameraParams,
		const char *name,
		bool shakable,
		SceneNodeId parentNode)
	{
		auto& s = g_sceneMgrPtr->getScene(sceneId);

		auto entityId = createNewSceneNode(sceneId, true, parentNode);
		auto sceneNodeId = s.entityManager->getEntityComponentId(entityId, scene::SceneNode::componentType);
		auto movementId  = s.entityManager->getEntityComponentId(entityId, scene::MovementComponent::componentType);

		scene::CameraInstance ci{};
		ci.sceneNodeId = sceneNodeId;
		ci.movementId = movementId;
		bool makePrimary = (s.cameras.size() == 0); // if this is the first camera in the scene, make it primary
		ci.cameraId = s.createCamera(cameraParams, makePrimary);
		strcpy_s(ci.name, sizeof(ci.name), name);

		auto camInstId = s.entityManager->addComponentToEntity(std::move(ci), entityId);

		if (shakable) {
			game::addScreenShakeNodeToCamera(s, entityId, camInstId);
		}

		return entityId;
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

	uint64_t griffin_scene_createDataComponentStore(uint64_t scene, uint16_t typeId,
													uint32_t componentSize, size_t reserve)
	{
		SceneId sceneId;
		sceneId.value = scene;

		auto& s = g_sceneMgrPtr->getScene(sceneId);
		return s.entityManager->createDataComponentStore(typeId, componentSize, reserve);
	}


	void* griffin_scene_getDataComponent(uint64_t scene, uint64_t component)
	{
		SceneId sceneId;
		sceneId.value = scene;
		ComponentId cmpId;
		cmpId.value = component;

		auto& s = g_sceneMgrPtr->getScene(sceneId);
		return s.entityManager->getDataComponent(cmpId);
	}


	uint64_t griffin_scene_addDataComponentToEntity(uint64_t scene, uint16_t typeId, uint64_t entity)
	{
		SceneId sceneId;
		sceneId.value = scene;
		EntityId entityId;
		entityId.value = entity;

		auto& s = g_sceneMgrPtr->getScene(sceneId);
		return s.entityManager->addDataComponentToEntity(typeId, entityId).value;
	}


	// Scene Node functions

	uint64_t griffin_scene_createNewSceneNode(
		uint64_t scene,
		bool movable,
		uint64_t parentNode)
	{
		try {
			SceneId sceneId{};
			sceneId.value = scene;
			SceneNodeId parentNodeId{};
			parentNodeId.value = parentNode;
			
			auto entityId = createNewSceneNode(sceneId, movable, parentNodeId);
			return entityId.value;
		}
		catch (std::exception ex) {
			logger.error("griffin_scene_createNewSceneNode: %s", ex.what());
		}
		return 0;
	}


	uint64_t griffin_scene_createNewModelInstance(
		uint64_t scene,
		uint64_t model,
		bool movable,
		uint64_t parentNode)
	{
		try {
			SceneId sceneId{};
			sceneId.value = scene;
			SceneNodeId parentNodeId{};
			parentNodeId.value = parentNode;
			Id_T modelId{};
			modelId.value = model;
			
			auto entityId = createNewModelInstance(sceneId, movable, /*modelId,*/ parentNodeId);
			return entityId.value;
		}
		catch (std::exception ex) {
			logger.error("griffin_scene_createNewModelInstance: %s", ex.what());
		}
		return 0;
	}

	
	uint64_t griffin_scene_createNewCamera(
		uint64_t scene,
		griffin_CameraParameters* cameraParams,
		const char *name,
		bool shakable,
		uint64_t parentNode)
	{
		try {
			SceneId sceneId{};
			sceneId.value = scene;
			SceneNodeId parentNodeId{};
			parentNodeId.value = parentNode;

			CameraParameters cp{};
			griffin_CameraParameters& inCp = *cameraParams;
			cp.nearClipPlane = inCp.nearClipPlane;
			cp.farClipPlane = inCp.farClipPlane;
			cp.viewportWidth = inCp.viewportWidth;
			cp.viewportHeight = inCp.viewportHeight;
			cp.verticalFieldOfViewDegrees = inCp.verticalFieldOfViewDegrees;
			cp.cameraType = inCp.cameraType;

			auto entityId = createNewCamera(sceneId, cp, name, shakable, parentNodeId);
			return entityId.value;
		}
		catch (std::exception ex) {
			logger.error("griffin_scene_createNewCamera: %s", ex.what());
		}

		return 0;
	}

	
	uint64_t griffin_scene_createLight(uint64_t scene, uint64_t parentEntity)
	{
		return 0;
	}


	// Position, Orientation, Translation, Rotation functions

/*	scene::SceneNode* getSceneNode(uint64_t scene, uint64_t entity)
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
			logger.error("griffin_scene_getSceneNode: %s", ex.what());
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
	}*/

#ifdef __cplusplus
}
#endif