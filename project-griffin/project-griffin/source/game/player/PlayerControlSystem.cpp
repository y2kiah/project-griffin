#include "PlayerControlSystem.h"
#include <scene/Scene.h>
#include <entity/EntityManager.h>
#include <input/InputSystem.h>
#include <game/impl/GameImpl.h>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <api/SceneApi.h>
#include <scene/Camera.h>


void griffin::game::PlayerControlSystem::updateFrameTick(Game* pGame, Engine& engine, const UpdateInfo& ui)
{
	using namespace glm;

	Game& game = *pGame;
	auto& scene = engine.sceneManager->getScene(game.sceneId);

	auto sceneNodeId = scene.entityManager->getEntityComponentId(playerId, scene::SceneNode::componentType);
	auto& node = scene.entityManager->getComponentStore<scene::SceneNode>().getComponent(sceneNodeId);

	// TODO: this all probably works for (0,1,0) worldup vector, but worldup changes with position on planet, need to make this work everywhere
	// if based on local coordinate system, maybe all I need to to is parent the player with a worldup aligned parent node, then localY up is
	// still relatively correct for the fps camera controls??

	// Mouse look
	if (pitchRaw != 0 || yawRaw != 0) {
		const float lookRate = 2.0f  * static_cast<float>(M_PI);

		float yawAngle = -yawMapped * lookRate * ui.deltaT;
		float pitchAngle = -pitchMapped * lookRate * ui.deltaT;
		
		// constrain pitch to +89/-90 degrees
		const float deg179 = radians(179.0f);
		float currentPitch = pitch(node.rotationLocal); // get pitch in parent node's space
		if (currentPitch + pitchAngle < 0) {
			pitchAngle = -currentPitch;
		}
		else if (currentPitch + pitchAngle >= deg179) {
			pitchAngle = deg179 - currentPitch;
		}

		node.rotationLocal = normalize(
								angleAxis(yawAngle, vec3(0, 0, 1.0f))
								* node.rotationLocal
								* angleAxis(pitchAngle, vec3(1.0f, 0, 0)));
		node.orientationDirty = 1;
	}

	// Camera movement
	{
		vec3 targetV = { 0, 0, 0 };

		if (moveForward != 0 || moveSide != 0) {
			auto camInstId = scene.entityManager->getEntityComponentId(playerId, scene::CameraInstanceContainer::componentType);
			auto& camInst = scene.entityManager->getComponentStore<scene::CameraInstanceContainer>().getComponent(camInstId);
			auto& cam = *scene.cameras[camInst.cameraId];

			// determine target velocity
			float speedTarget = 12.0f; // jog speed, in ft/s
			if (speedToggle == SpeedFlag_Sprint &&
				moveForward == 1) // can only sprint moving forward
			{
				speedTarget = 29.0f;
			}
			else if (speedToggle == SpeedFlag_Walk) {
				speedTarget = 5.0f;
			}
			speedTarget *= ui.deltaT; // speed per timestep

			vec3 forwardDir = normalize(vec3(cam.getViewDirection().x, cam.getViewDirection().y, 0));
			vec3 right = cross(forwardDir, cam.getWorldUp());

			targetV = forwardDir * static_cast<float>(moveForward);
			targetV += right * static_cast<float>(moveSide);

			targetV = normalize(targetV) * speedTarget;
		}

		// TODO: note that once collisions come into the play, the achieved velocity from last frame
		// might be different than the original velocity attempted. Verlet integration handles this

		// accelerate to the targetVelocity from current velocity
		auto deltaV = targetV - velocity; // difference between target and current
		float len2 = (deltaV.x * deltaV.x) + (deltaV.y * deltaV.y) + (deltaV.z * deltaV.z);
		if (len2 < epsilon<float>()) { // target velocity achieved
			velocity = targetV;
		}
		else { // accelerate toward target velocity
			// cap acceleration to the distance remaining to the target velocity
			float acceleration = glm::min(44.0f * ui.deltaT * ui.deltaT, sqrt(len2)); // ft/s^2
			velocity += normalize(deltaV) * acceleration;
		}

		node.translationLocal += velocity;
		node.positionDirty = 1;
	}

	// reset movement vars for next frame
	moveForward = moveSide = pitchRaw = yawRaw = 0;
	pitchMapped = yawMapped = 0.0f;
	speedToggle = SpeedFlag_Normal;
}


void griffin::game::PlayerControlSystem::init(Game* pGame, const Engine& engine, const SDLApplication& app)
{
	using namespace griffin::scene;
	Game& game = *pGame;
	auto& scene = engine.sceneManager->getScene(game.sceneId);

	// create player scene node
	playerId = createCamera(game.sceneId, NullId_T, CameraParameters{
		0.1f, 100000.0f,	// near/far clip
		app.getPrimaryWindow().width, app.getPrimaryWindow().height, // viewport
		60.0f, Camera_Perspective
	}, "player");

	// set up camera position and orientation
	auto camNode = scene.entityManager->getEntityComponentId(playerId, scene::SceneNode::componentType);
	auto& node = scene.entityManager->getComponentStore<scene::SceneNode>().getComponent(camNode);

	auto camInst = scene.entityManager->getEntityComponentId(playerId, scene::CameraInstanceContainer::componentType);
	auto& cam = scene.entityManager->getComponentStore<scene::CameraInstanceContainer>().getComponent(camInst);
	scene.cameras[cam.cameraId]->lookAt(vec3{ 0, 0, 6.0f }, vec3{ 1.0f, 0, 6.0f }, vec3{ 0, 0, 1.0f });

	// set scene node location and orientation to the camera's
	node.translationLocal = scene.cameras[cam.cameraId]->getEyePoint();
	node.positionDirty = 1;
	node.rotationLocal = scene.cameras[cam.cameraId]->getOrientation();
	node.orientationDirty = 1;

	// get playerfps input mapping ids
	{
		auto ctx = engine.inputSystem->getInputContextHandle("playerfps");
		playerfpsInputContextId = ctx;

		forwardId = engine.inputSystem->getInputMappingHandle("Move Forward", ctx);
		backId    = engine.inputSystem->getInputMappingHandle("Move Backward", ctx);
		leftId    = engine.inputSystem->getInputMappingHandle("Move Left", ctx);
		rightId   = engine.inputSystem->getInputMappingHandle("Move Right", ctx);
		sprintId  = engine.inputSystem->getInputMappingHandle("Sprint", ctx);
		walkId    = engine.inputSystem->getInputMappingHandle("Walk", ctx);
		lookXId   = engine.inputSystem->getInputMappingHandle("Mouse Look X", ctx);
		lookYId   = engine.inputSystem->getInputMappingHandle("Mouse Look Y", ctx);

		assert(forwardId != NullId_T && backId  != NullId_T &&
			   leftId    != NullId_T && rightId != NullId_T &&
			   sprintId  != NullId_T && walkId  != NullId_T &&
			   lookXId   != NullId_T && lookYId != NullId_T &&
			   "playerfps input mappings changed");
	}

	// playerfps input handlers
	{
		using namespace input;

		int priority = 0;

		engine.inputSystem->registerCallback(priority, [&engine, &game, this](FrameMappedInput& mi){

			auto& scene = engine.sceneManager->getScene(game.sceneId);

			engine.inputSystem->handleInputState(forwardId, mi, [this](MappedState& ms, InputContext& c){
				moveForward = glm::min(moveForward + 1, 1);
				return true;
			});

			engine.inputSystem->handleInputState(backId, mi, [this](MappedState& ms, InputContext& c){
				moveForward = glm::max(moveForward - 1, -1);
				return true;
			});

			engine.inputSystem->handleInputState(leftId, mi, [this](MappedState& ms, InputContext& c){
				moveSide = glm::max(moveSide - 1, -1);
				return true;
			});

			engine.inputSystem->handleInputState(rightId, mi, [this](MappedState& ms, InputContext& c){
				moveSide = glm::min(moveSide + 1, 1);
				return true;
			});

			engine.inputSystem->handleInputState(sprintId, mi, [this](MappedState& ms, InputContext& c){
				speedToggle = SpeedFlag_Sprint;
				return true;
			});

			engine.inputSystem->handleInputState(walkId, mi, [this](MappedState& ms, InputContext& c){
				speedToggle = SpeedFlag_Walk;
				return true;
			});

			engine.inputSystem->handleInputAxis(lookXId, mi, [this](MappedAxis& ma, InputContext& c){
				yawRaw += ma.axisMotion->relRaw;
				yawMapped += ma.axisMotion->relMapped;
				return true;
			});

			engine.inputSystem->handleInputAxis(lookYId, mi, [this](MappedAxis& ma, InputContext& c){
				pitchRaw += ma.axisMotion->relRaw;
				pitchMapped += ma.axisMotion->relMapped;
				return true;
			});
		});
	}
}