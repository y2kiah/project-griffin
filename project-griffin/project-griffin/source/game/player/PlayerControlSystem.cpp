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
	Game& game = *pGame;
	auto& scene = engine.sceneManager->getScene(game.sceneId);

	auto sceneNodeId = scene.entityManager->getEntityComponentId(playerId, scene::SceneNode::componentType);
	auto& node = scene.entityManager->getComponentStore<scene::SceneNode>().getComponent(sceneNodeId);

	// Mouse look
	if (pitchRaw != 0 || yawRaw != 0) {
		const float lookRate = 2.0f  * static_cast<float>(M_PI);

		float yawAngle = yawMapped * lookRate * ui.deltaT;
		float pitchAngle = pitchMapped * lookRate * ui.deltaT;

		glm::vec4 angles(0.0f, -pitchAngle, -yawAngle, 0.0f);

		node.rotationLocal = glm::quat(angles.xyz) * node.rotationLocal;
		glm::normalize(node.rotationLocal);
		node.orientationDirty = 1;
	}

	// Camera movement
	if (moveForward != 0 || moveSide != 0) { // TODO: this goes away when keys control acceleration instead of velocity
		auto camInstId = scene.entityManager->getEntityComponentId(playerId, scene::CameraInstanceContainer::componentType);
		auto& camInst = scene.entityManager->getComponentStore<scene::CameraInstanceContainer>().getComponent(camInstId);
		auto& cam = *scene.cameras[camInst.cameraId];

		float speedTarget = 10.0f; // in ft. per second
		if (speedToggle == SpeedFlag_Sprint) {
			speedTarget = 20.0f;
		}
		else if (speedToggle == SpeedFlag_Walk) {
			speedTarget = 5.0f;
		}

		glm::vec3 forwardDir = glm::normalize(glm::vec3(cam.getViewDirection().x, 0.0f, cam.getViewDirection().z));
		glm::vec3 right = glm::cross(forwardDir, cam.getWorldUp());
		
		glm::vec3 velocity = forwardDir * static_cast<float>(moveForward);
		velocity += right * static_cast<float>(moveSide);

		// TODO: we want to smoothly accelerate to the speedTarget from a separate speed
		velocity = glm::normalize(velocity) * speedTarget * ui.deltaT;

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

	// look at the origin
	auto camNode = scene.entityManager->getEntityComponentId(playerId, scene::SceneNode::componentType);
	auto& node = scene.entityManager->getComponentStore<scene::SceneNode>().getComponent(camNode);

	auto camInst = scene.entityManager->getEntityComponentId(playerId, scene::CameraInstanceContainer::componentType);
	auto& cam = scene.entityManager->getComponentStore<scene::CameraInstanceContainer>().getComponent(camInst);
	scene.cameras[cam.cameraId]->lookAt(glm::vec3{ 1.0f, 6.0f, 0 }, glm::vec3{ 0, 6.0f, 0 }, glm::vec3{ 0, 1.0f, 0 });

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
		backId = engine.inputSystem->getInputMappingHandle("Move Backward", ctx);
		leftId = engine.inputSystem->getInputMappingHandle("Move Left", ctx);
		rightId = engine.inputSystem->getInputMappingHandle("Move Right", ctx);
		sprintId = engine.inputSystem->getInputMappingHandle("Sprint", ctx);
		walkId = engine.inputSystem->getInputMappingHandle("Walk", ctx);
		lookXId = engine.inputSystem->getInputMappingHandle("Mouse Look X", ctx);
		lookYId = engine.inputSystem->getInputMappingHandle("Mouse Look Y", ctx);

		assert(forwardId != NullId_T && backId != NullId_T &&
			   leftId != NullId_T && rightId != NullId_T &&
			   sprintId != NullId_T && walkId != NullId_T &&
			   lookXId != NullId_T && lookYId != NullId_T &&
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