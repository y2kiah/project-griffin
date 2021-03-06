#include "DevCameraSystem.h"
#include <scene/Scene.h>
#include <entity/EntityManager.h>
#include <input/InputSystem.h>
#include <game/impl/GameImpl.h>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <api/SceneApi.h>
#include <scene/Camera.h>

using namespace glm;

const dvec3 c_xAxis(1.0, 0.0, 0.0);
const dvec3 c_yAxis(0.0, 1.0, 0.0);
const dvec3 c_zAxisNeg(0.0, 0.0, -1.0);


void griffin::game::DevCameraSystem::updateFrameTick(Game& game, Engine& engine, const UpdateInfo& ui)
{
	auto& scene = engine.sceneManager->getScene(game.sceneId);
	auto& entityMgr = *scene.entityManager;

	auto& move = entityMgr.getComponent<scene::MovementComponent>(movementComponentId);
	auto& node = entityMgr.getComponent<scene::SceneNode>(move.sceneNodeId);

	// Mouse look
	if (pitchRaw != 0 || yawRaw != 0 || roll != 0) {
		constexpr float rollRate = 0.25f * pi<float>();
		constexpr float lookRate = 2.0f  * pi<float>();

		float yawAngle = yawMapped * lookRate * ui.deltaT;
		float pitchAngle = pitchMapped * lookRate * ui.deltaT;
		float rollAngle = roll * rollRate * ui.deltaT;

		dvec3 angles(-pitchAngle, -yawAngle, -rollAngle);
		angles = angles * inverse(node.orientationWorld); // transform angles into viewspace

		move.prevRotation = move.nextRotation;
		move.nextRotation = normalize(dquat(angles) * move.prevRotation);
		move.prevRotationDirty = move.rotationDirty;
		move.rotationDirty = 1;
	}
	else {
		move.prevRotation = move.nextRotation;
		move.prevRotationDirty = move.rotationDirty;
		move.rotationDirty = 0;
	}

	// Camera movement
	if (moveForward != 0 || moveSide != 0 || moveVertical != 0) {
		vec3 viewDir = node.orientationWorld * c_zAxisNeg;
		vec3 right   = node.orientationWorld * c_xAxis;
		vec3 up      = node.orientationWorld * c_yAxis;

		vec3 velocity = viewDir * static_cast<float>(moveForward);
		velocity += right * static_cast<float>(moveSide);
		velocity += up * static_cast<float>(moveVertical);

		velocity = normalize(velocity) * speed * (!speedToggle ? 3.0f : 1.0f) * ui.deltaT;

		move.prevTranslation = move.nextTranslation;
		move.nextTranslation += velocity;
		move.prevTranslationDirty = move.translationDirty;
		move.translationDirty = 1;
	}
	else {
		move.prevTranslation = move.nextTranslation;
		move.prevTranslationDirty = move.translationDirty;
		move.translationDirty = 0;
	}

	// reset movement vars for next frame
	moveForward = moveSide = moveVertical = roll = pitchRaw = yawRaw = 0;
	pitchMapped = yawMapped = 0.0f;
	speedToggle = false;
}


void griffin::game::DevCameraSystem::init(Game& game, const Engine& engine, const SDLApplication& app)
{
	using namespace griffin::scene;

	auto& scene = engine.sceneManager->getScene(game.sceneId);
	auto& entityMgr = *scene.entityManager;

	// create dev camera scene node
	devCameraId = createNewCamera(game.sceneId, CameraParameters{
		0.1f, 53000000.0f,	// near/far clip
		app.getPrimaryWindow().width, app.getPrimaryWindow().height, // viewport
		60.0f, Camera_Perspective
	}, "devcamera");

	// look at the origin
	auto& cam = *entityMgr.getEntityComponent<scene::CameraInstance>(devCameraId);
	movementComponentId = cam.movementId;

	auto& move = entityMgr.getComponent<scene::MovementComponent>(movementComponentId);
	auto& node = entityMgr.getComponent<scene::SceneNode>(move.sceneNodeId);

	scene.cameras[cam.cameraId]->lookAt(vec3{ 120.0f, 0, 40.0f }, vec3{ 0, 0, 0 }, vec3{ 0, 0, 1.0f });

	// set scene node location and orientation to the camera's
	node.translationLocal = scene.cameras[cam.cameraId]->getEyePoint();
	node.positionDirty = 1;
	move.prevTranslation = move.nextTranslation = node.translationLocal;

	node.rotationLocal = scene.cameras[cam.cameraId]->getOrientation();
	node.orientationDirty = 1;
	move.prevRotation = move.nextRotation = node.rotationLocal;

	// get devcamera input mapping ids
	{
		auto ingameCtx = engine.inputSystem->getInputContextHandle("ingame");
		toggleId = engine.inputSystem->getInputMappingHandle("Toggle Dev Camera", ingameCtx);

		playerfpsInputContextId = engine.inputSystem->getInputContextHandle("playerfps");

		auto ctx = engine.inputSystem->getInputContextHandle("devcamera");
		devCameraInputContextId = ctx;

		forwardId     = engine.inputSystem->getInputMappingHandle("Move Forward", ctx);
		backId        = engine.inputSystem->getInputMappingHandle("Move Backward", ctx);
		leftId        = engine.inputSystem->getInputMappingHandle("Move Left", ctx);
		rightId       = engine.inputSystem->getInputMappingHandle("Move Right", ctx);
		upId          = engine.inputSystem->getInputMappingHandle("Move Up", ctx);
		downId        = engine.inputSystem->getInputMappingHandle("Move Down", ctx);
		rollLeftId    = engine.inputSystem->getInputMappingHandle("Roll Left", ctx);
		rollRightId   = engine.inputSystem->getInputMappingHandle("Roll Right", ctx);
		speedToggleId = engine.inputSystem->getInputMappingHandle("Speed", ctx);
		lookXId       = engine.inputSystem->getInputMappingHandle("Mouse Look X", ctx);
		lookYId       = engine.inputSystem->getInputMappingHandle("Mouse Look Y", ctx);
		speedScrollIncreaseId = engine.inputSystem->getInputMappingHandle("Speed Scroll Increase", ctx);
		speedScrollDecreaseId = engine.inputSystem->getInputMappingHandle("Speed Scroll Decrease", ctx);

		assert(forwardId     != NullId_T && backId      != NullId_T &&
			   leftId        != NullId_T && rightId     != NullId_T &&
			   upId          != NullId_T && downId      != NullId_T &&
			   rollLeftId    != NullId_T && rollRightId != NullId_T &&
			   speedToggleId != NullId_T && lookXId     != NullId_T &&
			   lookYId       != NullId_T && toggleId    != NullId_T &&
			   speedScrollIncreaseId != NullId_T && speedScrollDecreaseId != NullId_T &&
			   "devcamera input mappings changed");
	}

	// devcamera input handlers
	{
		using namespace input;

		// set the callback priority to the same as input context priority
		int priority = engine.inputSystem->getContext(devCameraInputContextId).priority;

		engine.inputSystem->registerCallback(priority, [&engine, &game, this](FrameMappedInput& mi){

			auto& scene = engine.sceneManager->getScene(game.sceneId);

			/**
			* Handle ingame context's devcamera toggle
			*/
			engine.inputSystem->handleInputAction(toggleId, mi, [this, &engine, &scene](MappedAction& ma, InputContext& c){
				active = !active;
				
				engine.inputSystem->setContextActive(devCameraInputContextId, active);
				engine.inputSystem->setContextActive(playerfpsInputContextId, !active);

				if (active) {
					toggleActiveCamera = scene.getActiveCamera(); // save the camera to go back to
					auto pCamInst = scene.entityManager->getEntityComponent<scene::CameraInstance>(devCameraId);
					scene.setActiveCamera(pCamInst->cameraId);
				}
				else {
					scene.setActiveCamera(toggleActiveCamera);
				}
				return true;
			});

			/**
			* Handle movement states
			*/
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

			engine.inputSystem->handleInputState(upId, mi, [this](MappedState& ms, InputContext& c){
				moveVertical = glm::min(moveVertical + 1, 1);
				return true;
			});

			engine.inputSystem->handleInputState(downId, mi, [this](MappedState& ms, InputContext& c){
				moveVertical = glm::max(moveVertical - 1, -1);
				return true;
			});

			engine.inputSystem->handleInputState(rollLeftId, mi, [this](MappedState& ms, InputContext& c){
				roll = glm::max(roll - 1, -1);
				return true;
			});

			engine.inputSystem->handleInputState(rollRightId, mi, [this](MappedState& ms, InputContext& c){
				roll = glm::min(roll + 1, 1);
				return true;
			});

			engine.inputSystem->handleInputState(speedToggleId, mi, [this](MappedState& ms, InputContext& c){
				speedToggle = true;
				return true;
			});

			/**
			* Handle speed scroll with mousewheel
			*/
			engine.inputSystem->handleInputAction(speedScrollIncreaseId, mi, [this](MappedAction& ma, InputContext& c){
				speed *= 2.0f;
				return true;
			});

			engine.inputSystem->handleInputAction(speedScrollDecreaseId, mi, [this](MappedAction& ma, InputContext& c){
				speed *= 0.5f;
				return true;
			});

			/**
			* Handle Mouse-Look
			*/
			engine.inputSystem->handleInputAxis(lookXId, mi, [this](MappedAxis& ma, InputContext& c){
				/*logger.verbose(Logger::Category_Input, "devcamera axis %s,  handled motion, relRaw=%d, relMapped=%0.1f",
						ma.inputMapping->name, ma.axisMotion->relRaw, ma.axisMotion->relMapped);*/
				yawRaw += ma.axisMotion->relRaw;
				yawMapped += ma.axisMotion->relMapped;
				return true;
			});

			engine.inputSystem->handleInputAxis(lookYId, mi, [this](MappedAxis& ma, InputContext& c){
				/*logger.verbose(Logger::Category_Input, "devcamera axis %s,  handled motion, relRaw=%d, relMapped=%0.1f",
						ma.inputMapping->name, ma.axisMotion->relRaw, ma.axisMotion->relMapped);*/
				pitchRaw += ma.axisMotion->relRaw;
				pitchMapped += ma.axisMotion->relMapped;
				return true;
			});
		});
	}
}