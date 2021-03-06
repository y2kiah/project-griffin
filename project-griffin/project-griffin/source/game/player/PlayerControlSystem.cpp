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
#include <utility/blend.h>


#define playerHeight			6.0f	// ft
#define sprintSpeed				27.0f	// ft/s
#define jogSpeed				12.0f	// ft/s
#define walkSpeed				5.0f	// ft/s
#define movementAcceleration	60.0f	// ft/s^2
#define sprintStride			13.5f	// ft
#define walkStride				2.5f	// ft
#define headBobDrop				0.1f	// ft
#define crouchHeight			3.0f	// ft
#define crouchRate				2.0f	// 1/s

using namespace glm;

const dvec3 c_zAxisNeg(0.0, 0.0, -1.0);


void griffin::game::PlayerControlSystem::updateFrameTick(Game& game, Engine& engine, const UpdateInfo& ui)
{
	auto& scene = engine.sceneManager->getScene(game.sceneId);
	auto& entityMgr = *scene.entityManager;

	auto& move = entityMgr.getComponent<scene::MovementComponent>(movementComponentId);
	auto& node = entityMgr.getComponent<scene::SceneNode>(move.sceneNodeId);

	// TODO: this all probably works for (0,1,0) (or (0,0,1)??) worldup vector, but worldup changes with position on planet, need to make this work everywhere
	// if based on local coordinate system, maybe all I need to to is parent the player with a worldup aligned parent node, then localY up is
	// still relatively correct for the fps camera controls??

	// Mouse look
	if (pitchRaw != 0 || yawRaw != 0) {
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

	// Player movement
	{
		vec3 targetV = { 0, 0, 0 };

		if (moveForward != 0 || moveSide != 0) {
			// determine target velocity
			float speedTarget = (crouching ? walkSpeed : jogSpeed); // default to jog speed, or walk when crouching

			// can only sprint moving forward and not crouching
			if (speedToggle == SpeedFlag_Sprint &&
				moveForward == 1 &&
				!crouching)
			{
				speedTarget = sprintSpeed;
			}
			else if (speedToggle == SpeedFlag_Walk) {
				speedTarget = walkSpeed;
			}
			speedTarget *= ui.deltaT; // speed per timestep

			vec3 viewDir = node.orientationWorld * c_zAxisNeg;
			vec3 forwardDir = normalize(vec3(viewDir.x, viewDir.y, 0.0f));
			vec3 right = cross(forwardDir, worldUp);

			targetV = forwardDir * static_cast<float>(moveForward);
			targetV += right * static_cast<float>(moveSide);

			targetV = normalize(targetV) * speedTarget;
		}

		// TODO: note that once collisions come into play, the achieved velocity from last frame
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

		// calculate crouch height
		// TODO: collision detection above the bounding sphere should keep the player crouching
		//	even though they've toggled crouching off, until they are free of the obstacle above

		float crouchZ = 0.0f; // crouch distance subtracted from player's height
		{
			float crouchDeltaT = (crouching ? 1.0f : -1.0f) * crouchRate * ui.deltaT;
			crouchT = clamp(crouchT + crouchDeltaT, 0.0f, 1.0f);
			crouchZ = mix(0.0f, playerHeight - crouchHeight, hermite(crouchT));
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

			headBobZ = sinf(headBob) * headBobDrop;
		}

		// update movement component values
		move.prevTranslation = move.nextTranslation;
		move.nextTranslation += velocity;
		move.nextTranslation.z = playerHeight - crouchZ - headBobZ;
		move.prevTranslationDirty = move.translationDirty;
		move.translationDirty = (move.prevTranslation == move.nextTranslation) ? 0 : 1;
	}

	// reset movement vars for next frame
	moveForward = moveSide = pitchRaw = yawRaw = 0;
	pitchMapped = yawMapped = 0.0f;
	speedToggle = SpeedFlag_Normal;
}


void griffin::game::PlayerControlSystem::init(Game& game, const Engine& engine, const SDLApplication& app)
{
	using namespace griffin::scene;

	auto& scene = engine.sceneManager->getScene(game.sceneId);
	auto& entityMgr = *scene.entityManager;

	worldUp = vec3(0.0f, 0.0f, 1.0f);

	// create player scene node
	playerId = createNewCamera(game.sceneId, CameraParameters{
		0.1f, 53000000.0f,	// near/far clip
		app.getPrimaryWindow().width, app.getPrimaryWindow().height, // viewport
		60.0f, Camera_Perspective
	}, "player");

	auto& cam = *entityMgr.getEntityComponent<scene::CameraInstance>(playerId);
	movementComponentId = cam.movementId;
	auto& move = entityMgr.getComponent<scene::MovementComponent>(movementComponentId);
	auto& node = entityMgr.getComponent<scene::SceneNode>(move.sceneNodeId);

	// set up camera position and orientation
	scene.cameras[cam.cameraId]->lookAt(vec3{ 0, 0, playerHeight }, vec3{ 1.0f, 0, playerHeight }, worldUp);

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

		// set the callback priority to the same as input context priority
		int priority = engine.inputSystem->getContext(playerfpsInputContextId).priority;

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

			bool crouchActive = engine.inputSystem->handleInputState(crouchId, mi, [this](MappedState& ms, InputContext& c){
				return true;
			});
			crouching = crouchActive;

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