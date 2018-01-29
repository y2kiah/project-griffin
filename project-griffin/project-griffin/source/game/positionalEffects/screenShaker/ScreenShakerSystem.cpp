#include "ScreenShakerSystem.h"
#include <scene/Scene.h>
#include <entity/EntityManager.h>
//#include <input/InputSystem.h>
#include <game/impl/GameImpl.h>
//#include <glm/vec3.hpp>
//#include <glm/gtc/quaternion.hpp>

//#include <api/SceneApi.h>
//#include <scene/Camera.h>
//#include <cmath>
//#include <utility/blend.h>


void griffin::game::ScreenShakerSystem::updateFrameTick(Game& game, Engine& engine, const UpdateInfo& ui)
{
	using namespace glm;

	auto& scene = engine.sceneManager->getScene(game.sceneId);
	auto& move = scene.entityManager->getComponent<scene::MovementComponent>(movementComponentId);

	// shake = turbulence * turbulence; // * turbulence
	// yaw   = maxYaw   * shake * getPerlinNoise(seed,   time, ...?);
	// pitch = maxPitch * shake * getPerlinNoise(seed=1, time, ...?);
	// roll  = maxRoll  * shake * getPerlinNoise(seed=2, time, ...?);

	// Mouse look
/*	if (pitchRaw != 0 || yawRaw != 0) {
		const double lookRate = 2.0  * M_PI;

		double yawAngle = -yawMapped * lookRate * ui.deltaT;
		double pitchAngle = -pitchMapped * lookRate * ui.deltaT;

		// constrain pitch to +/-89 degrees
		const double deg1 = radians(1.0);
		const double deg179 = radians(179.0);

		move.prevRotation = move.nextRotation;

		double currentPitch = pitch(move.prevRotation); // get pitch in parent node's space
		if (currentPitch + pitchAngle < deg1) {
			pitchAngle = deg1 - currentPitch;
		}
		else if (currentPitch + pitchAngle >= deg179) {
			pitchAngle = deg179 - currentPitch;
		}

		move.nextRotation = normalize(
			angleAxis(yawAngle, dvec3(0, 0, 1.0))
			* move.prevRotation
			* angleAxis(pitchAngle, dvec3(1.0, 0, 0)));

		move.prevRotationDirty = move.rotationDirty;
		move.rotationDirty = 1;
	}
	else {
		move.prevRotation = move.nextRotation;
		move.prevRotationDirty = move.rotationDirty;
		move.rotationDirty = 0;
	}
*/
}


void griffin::game::ScreenShakerSystem::init(Game& game, const Engine& engine, const SDLApplication& app)
{
	using namespace griffin::scene;
	auto& scene = engine.sceneManager->getScene(game.sceneId);

	playerId = game.player.playerId;
	movementComponentId = scene.entityManager->getEntityComponentId(playerId, scene::MovementComponent::componentType);

	playerfpsInputContextId = game.player.playerfpsInputContextId;

	
	// create game component stores for this system
	game.gameComponentStoreIds[ScreenShakerComponentTypeId] = scene.entityManager->createDataComponentStore(
		ScreenShakerComponentTypeId,
		sizeof(ScreenShakerComponent), 1);

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