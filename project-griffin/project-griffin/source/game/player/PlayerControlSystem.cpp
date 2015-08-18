#include "PlayerControlSystem.h"
#include <scene/Scene.h>
#include <entity/EntityManager.h>
#include <input/InputSystem.h>
#include <game/impl/GameImpl.h>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <api/SceneApi.h>
#include <scene/Camera.h>
#include <cmath>


#define playerHeight			6.0f	// ft/s
#define sprintSpeed				27.0f	// ft/s
#define jogSpeed				12.0f	// ft/s
#define walkSpeed				5.0f	// ft/s
#define movementAcceleration	60.0f	// ft/s^2
#define sprintStride			13.5f	// ft
#define walkStride				2.5f	// ft
#define headBobDrop				0.0833f	// ft


void griffin::game::PlayerControlSystem::updateFrameTick(Game* pGame, Engine& engine, const UpdateInfo& ui)
{
	using namespace glm;

	Game& game = *pGame;
	auto& scene = engine.sceneManager->getScene(game.sceneId);
	auto& move = scene.entityManager->getComponent<scene::MovementComponent>(movementComponentId);

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
		
		move.prevRotation = move.nextRotation;
		
		float currentPitch = pitch(move.prevRotation); // get pitch in parent node's space
		if (currentPitch + pitchAngle < 0) {
			pitchAngle = -currentPitch;
		}
		else if (currentPitch + pitchAngle >= deg179) {
			pitchAngle = deg179 - currentPitch;
		}

		move.nextRotation = normalize(
								angleAxis(yawAngle, vec3(0, 0, 1.0f))
								* move.prevRotation
								* angleAxis(pitchAngle, vec3(1.0f, 0, 0)));
		move.rotationDirty = 1;
	}
	else {
		move.rotationDirty = 0;
	}

	// Camera movement
	{
		vec3 targetV = { 0, 0, 0 };

		if (moveForward != 0 || moveSide != 0) {
			auto camInstId = scene.entityManager->getEntityComponentId(playerId, scene::CameraInstanceContainer::componentType);
			auto& camInst = scene.entityManager->getComponentStore<scene::CameraInstanceContainer>().getComponent(camInstId);
			auto& cam = *scene.cameras[camInst.cameraId];

			// determine target velocity
			float speedTarget = jogSpeed; // jog speed, in ft/s
			if (speedToggle == SpeedFlag_Sprint &&
				moveForward == 1) // can only sprint moving forward
			{
				speedTarget = sprintSpeed;
			}
			else if (speedToggle == SpeedFlag_Walk) {
				speedTarget = walkSpeed;
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
			float acceleration = glm::min(movementAcceleration * ui.deltaT * ui.deltaT, sqrt(len2)); // ft/s^2
			velocity += normalize(deltaV) * acceleration;
		}

		// calculate head bob height, we assume a walking stride of 2.5ft per step and a peak fall of 4 inches
		float headBobZ = 0.0f;
		{
			float frameStep = length(velocity);
			
			// add to head bob due to movement velocity
			if (frameStep != 0.0f) {
				float frameSprintSpeed = sprintSpeed * ui.deltaT;

				// adjust head bob stride based on movement speed
				float pct = frameStep / frameSprintSpeed;
				float stride = max(pct * sprintStride, walkStride);

				// normalize this frame's step distance according to head bob stride
				float headBobInterval = frameStep / stride;

				headBob += (float)M_PI * headBobInterval;
				// use only the first PI radians of the sine wave
				while (headBob >= (float)M_PI) {
					headBob -= (float)M_PI;
				}
			}
			// add to head bob due to standing up straight when not moving
			else if (headBob != 0.0f) {
				float standUpInterval = (float)M_PI * 2 * ui.deltaT;
				// take the quickest route back to zero, plus or minus
				if (headBob > (float)M_PI * 0.5f) {
					headBob += standUpInterval;
					if (headBob >= (float)M_PI) {
						headBob = 0.0f;
					}
				}
				else {
					headBob -= standUpInterval;
					if (headBob < 0) {
						headBob = 0.0f;
					}
				}
			}

			headBobZ = playerHeight - (sinf(headBob) * headBobDrop);
		}

		// update movement component values
		move.prevTranslation = move.nextTranslation;
		move.nextTranslation += velocity;
		move.nextTranslation.z = headBobZ;
		move.translationDirty = 1;
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

	movementComponentId = scene.entityManager->getEntityComponentId(playerId, scene::MovementComponent::componentType);

	// set up camera position and orientation
	auto pNode    = scene.entityManager->getEntityComponent<scene::SceneNode>(playerId);
	auto pCamInst = scene.entityManager->getEntityComponent<scene::CameraInstanceContainer>(playerId);
	
	assert(pNode != nullptr && pCamInst != nullptr && movementComponentId != NullId_T);
	auto &node = *pNode;
	auto &cam = *pCamInst;
	auto& move = scene.entityManager->getComponent<scene::MovementComponent>(movementComponentId);

	scene.cameras[cam.cameraId]->lookAt(vec3{ 0, 0, playerHeight }, vec3{ 1.0f, 0, playerHeight }, vec3{ 0, 0, 1.0f });

	// set scene node location and orientation to the camera's
	node.translationLocal = scene.cameras[cam.cameraId]->getEyePoint();
	node.positionDirty = 1;
	move.prevTranslation = move.nextTranslation = node.translationLocal;

	node.rotationLocal = scene.cameras[cam.cameraId]->getOrientation();
	node.orientationDirty = 1;
	move.prevRotation = move.nextRotation = node.rotationLocal;

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
		crouchId  = engine.inputSystem->getInputMappingHandle("Crouch", ctx);
		lookXId   = engine.inputSystem->getInputMappingHandle("Mouse Look X", ctx);
		lookYId   = engine.inputSystem->getInputMappingHandle("Mouse Look Y", ctx);

		assert(forwardId != NullId_T && backId  != NullId_T &&
			   leftId    != NullId_T && rightId != NullId_T &&
			   sprintId  != NullId_T && walkId  != NullId_T &&
			   crouchId  != NullId_T &&
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

			//engine.inputSystem->handleInputState(crouchId, mi, [this](MappedState& ms, InputContext& c){
				
			//	return true;
			//});

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