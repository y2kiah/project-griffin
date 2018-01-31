#include "ScreenShakerSystem.h"
#include <scene/Scene.h>
#include <entity/EntityManager.h>
//#include <input/InputSystem.h>
#include <game/impl/GameImpl.h>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>


void griffin::game::ScreenShakerSystem::updateFrameTick(
	Game& game,
	Engine& engine,
	const UpdateInfo& ui)
{
	using namespace glm;

	auto& scene = engine.sceneManager->getScene(game.sceneId);
	auto& entityMgr = *scene.entityManager;

	auto& producers = entityMgr.getComponentStore<ScreenShakeProducer>();
	auto& shakeNodes = entityMgr.getComponentStore<ScreenShakeNode>();

	// for each ScreenShakeNode (receiver)
	for (auto& n : shakeNodes.getComponents()) {
		auto& shakeNode = n.component;
		auto& shakeSceneNode = entityMgr.getComponent<scene::SceneNode>(shakeNode.sceneNodeId);
		
		shakeNode.prevTurbulence = shakeNode.nextTurbulence;
		shakeNode.nextTurbulence = 0;

		// for each ScreenShakeProducer component
		for (auto& p : producers.getComponents()) {
			auto& producer = p.component;

			// TODO: to support > 1 SceneNode per entity, convert this to a function on an entity
			auto pNode = entityMgr.getEntityComponent<scene::SceneNode>(p.entityId);
			assert(pNode != nullptr && "entity expected to have a SceneNode");
			auto& producerSceneNode = *pNode;

			float producerRadiusSq = producer.radius * producer.radius;

			vec3 nodeToProducer = shakeSceneNode.positionWorld - producerSceneNode.positionWorld;
			float distSq = dot(nodeToProducer, nodeToProducer);

			// determine the effectiveness based on distance ratio to radius of the producer
			float effectiveTurbulence = (producerRadiusSq == 0.0f)
				? producer.turbulence
				: producer.turbulence * glm::max((producerRadiusSq - distSq) / producerRadiusSq, 0.0f);

			// accumulate effective turbulence level from all producers
			shakeNode.nextTurbulence += effectiveTurbulence;
		}
	}

	// for each ScreenShakeProducer component
	for (auto& p : producers.getComponents()) {
		auto& producer = p.component;

		// decrease the turbulence linearly by time, unless TTL is 0 (which means it stays constant forever, probably controlled externally)
		float turbulenceFalloff = (producer.totalTimeToLiveMS == 0) ? 0
			: ui.deltaMs / producer.totalTimeToLiveMS;

		producer.turbulence -= turbulenceFalloff;
	}
	
	// remove expired components (consider adding an autoremove flag to control this)
	entityMgr.removeComponent_if<ScreenShakeProducer>([](const ScreenShakeProducer& producer) {
		return (producer.turbulence <= 0.0f);
	});
}


void griffin::game::ScreenShakerSystem::renderFrameTick(
	Game& game,
	Engine& engine,
	float interpolation,
	const int64_t realTime,
	const int64_t countsPassed)
{
	//auto camInstId = scene.entityManager->getEntityComponentId(devCameraId, scene::CameraInstance::componentType);
	//auto& camInst = scene.entityManager->getComponentStore<scene::CameraInstance>().getComponent(camInstId);
	//auto& cam = *scene.cameras[camInst.cameraId];

	// float turbulence = glm::mix(shakeNode.prevTurbulence, shakeNode.nextTurbulence, interpolation);
	// float shake = turbulence * turbulence; // * turbulence;

	// yaw   = maxYaw   * shake * getPerlinNoise(seed,   time, ...?);
	// pitch = maxPitch * shake * getPerlinNoise(seed=1, time, ...?);
	// roll  = maxRoll  * shake * getPerlinNoise(seed=2, time, ...?);
}


void griffin::game::ScreenShakerSystem::init(
	Game& game,
	const Engine& engine,
	const SDLApplication& app)
{
	using namespace griffin::scene;
	auto& scene = engine.sceneManager->getScene(game.sceneId);

	playerId = game.player.playerId;

	//playerfpsInputContextId = game.player.playerfpsInputContextId;

	
	// create game component stores for this system
	/*game.gameComponentStoreIds[ScreenShakerComponentTypeId] = scene.entityManager->createDataComponentStore(
		ScreenShakerComponentTypeId,
		sizeof(ScreenShakerComponent), 1);*/

	///// TEMP the devcamera store is not needed, demonstration
	// add devcamera movement component
	//devCameraMovementId = scene.entityManager->addDataComponentToEntity(DevCameraMovementComponentTypeId,
	//																	  devCameraId);

	//auto devCamMove = (DevCameraMovementComponent*)scene.entityManager->getDataComponentData(devCameraMovementId);
	///// end TEMP


/*
	node.rotationLocal = scene.cameras[cam.cameraId]->getOrientation();
	node.orientationDirty = 1;
	move.prevRotation = move.nextRotation = node.rotationLocal;

	// get playerfps input mapping ids
	{
		auto ctx = engine.inputSystem->getInputContextHandle("playerfps");
		playerfpsInputContextId = ctx;

		lookYId = engine.inputSystem->getInputMappingHandle("Mouse Look Y", ctx);

		assert(forwardId != NullId_T && backId != NullId_T &&
			leftId != NullId_T && rightId != NullId_T &&
			sprintId != NullId_T && walkId != NullId_T &&
			crouchId != NullId_T &&
			lookXId != NullId_T && lookYId != NullId_T &&
			"playerfps input mappings changed");
	}

	// playerfps input handlers
	{
		using namespace input;

		// set the callback priority to the same as input context priority
		int priority = engine.inputSystem->getContext(playerfpsInputContextId).priority;

		engine.inputSystem->registerCallback(priority, [&engine, &game, this](FrameMappedInput& mi) {

			auto& scene = engine.sceneManager->getScene(game.sceneId);

			engine.inputSystem->handleInputState(forwardId, mi, [this](MappedState& ms, InputContext& c) {
				moveForward = glm::min(moveForward + 1, 1);
				return true;
			});
		});
	}
*/
}