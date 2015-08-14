#include "DevCameraSystem.h"
#include <scene/Scene.h>
#include <entity/EntityManager.h>
#include <input/InputSystem.h>
#include <game/impl/GameImpl.h>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <api/SceneApi.h>
#include <scene/Camera.h>


void griffin::game::DevCameraSystem::updateFrameTick(Game* pGame, Engine& engine, const UpdateInfo& ui)
{
	using namespace glm;

	// check if there's any input to handle
	if (moveForward == 0 && moveSide == 0 && moveVertical == 0 &&
		roll == 0 && pitchRaw == 0 && yawRaw == 0)
	{
		return;
	}

	Game& game = *pGame;
	auto& scene = engine.sceneManager->getScene(game.sceneId);

	auto sceneNodeId = scene.entityManager->getEntityComponentId(devCameraId, scene::SceneNode::componentType);
	auto& node = scene.entityManager->getComponentStore<scene::SceneNode>().getComponent(sceneNodeId);

	// Mouse look
	if (pitchRaw != 0 || yawRaw != 0 || roll != 0) {
		const float rollRate = 0.25f * pi<float>();
		const float lookRate = 2.0f  * pi<float>();

		float yawAngle = yawMapped * lookRate * ui.deltaT;
		float pitchAngle = pitchMapped * lookRate * ui.deltaT;
		float rollAngle = roll * rollRate * ui.deltaT;

		vec3 angles(-pitchAngle, -yawAngle, -rollAngle);
		angles = angles * inverse(node.orientationWorld); // transform angles into viewspace

		node.rotationLocal = normalize(quat(angles) * node.rotationLocal);
		node.orientationDirty = 1;
	}

	// Camera movement
	if (moveForward != 0 || moveSide != 0 || moveVertical != 0) {
		auto camInstId = scene.entityManager->getEntityComponentId(devCameraId, scene::CameraInstanceContainer::componentType);
		auto& camInst = scene.entityManager->getComponentStore<scene::CameraInstanceContainer>().getComponent(camInstId);
		auto& cam = *scene.cameras[camInst.cameraId];
		
		const float speed = 10.0f; // in ft. per second

		vec3 viewDir = cam.getViewDirection();
		vec3 right = cam.getRightVector();//cross(cam.getViewDirection(), cam.getWorldUp());
		vec3 up = cam.getUpVector();//cross(viewDir, right);

		vec3 velocity = viewDir * static_cast<float>(moveForward);
		velocity += right * static_cast<float>(moveSide);
		velocity += up * static_cast<float>(moveVertical);

		velocity = normalize(velocity) * speed * (!speedToggle ? 3.0f : 1.0f) * ui.deltaT;

		node.translationLocal += velocity;
		node.positionDirty = 1;
	}

	// reset movement vars for next frame
	moveForward = moveSide = moveVertical = roll = pitchRaw = yawRaw = 0;
	pitchMapped = yawMapped = 0.0f;
	speedToggle = false;
}


void griffin::game::DevCameraSystem::init(Game* pGame, const Engine& engine, const SDLApplication& app)
{
	using namespace griffin::scene;
	Game& game = *pGame;
	auto& scene = engine.sceneManager->getScene(game.sceneId);

	// create game component stores for this system
	///// TEMP the devcamera store is not needed, only one of these things, just testing the waters
	game.gameComponentStoreIds[DevCameraMovementComponentTypeId] = scene.entityManager->createScriptComponentStore(
		DevCameraMovementComponentTypeId,
		sizeof(DevCameraMovementComponent), 1);
	///// end TEMP

	// create dev camera scene node
	devCameraId = createCamera(game.sceneId, NullId_T, CameraParameters{
		0.1f, 100000.0f,	// near/far clip
		app.getPrimaryWindow().width, app.getPrimaryWindow().height, // viewport
		60.0f, Camera_Perspective
	}, "devcamera");

	///// TEMP the devcamera store is not needed, demonstration
	// add devcamera movement component
	devCameraMovementId = scene.entityManager->addScriptComponentToEntity(DevCameraMovementComponentTypeId,
																		  devCameraId);

	auto devCamMove = (DevCameraMovementComponent*)scene.entityManager->getScriptComponentData(devCameraMovementId);
	///// end TEMP

	// look at the origin
	auto camNode = scene.entityManager->getEntityComponentId(devCameraId, scene::SceneNode::componentType);
	auto& node = scene.entityManager->getComponentStore<scene::SceneNode>().getComponent(camNode);

	auto camInst = scene.entityManager->getEntityComponentId(devCameraId, scene::CameraInstanceContainer::componentType);
	auto& cam = scene.entityManager->getComponentStore<scene::CameraInstanceContainer>().getComponent(camInst);
	scene.cameras[cam.cameraId]->lookAt(vec3{ 120.0f, 0, 40.0f }, vec3{ 0, 0, 0 }, vec3{ 0, 0, 1.0f });

	// set scene node location and orientation to the camera's
	node.translationLocal = scene.cameras[cam.cameraId]->getEyePoint();
	node.positionDirty = 1;
	node.rotationLocal = scene.cameras[cam.cameraId]->getOrientation();
	node.orientationDirty = 1;

	// get devcamera input mapping ids
	{
		auto ctx = engine.inputSystem->getInputContextHandle("devcamera");
		devCameraInputContextId = ctx;

		toggleId      = engine.inputSystem->getInputMappingHandle("Toggle", ctx);
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

		assert(forwardId     != NullId_T && backId      != NullId_T &&
			   leftId        != NullId_T && rightId     != NullId_T &&
			   upId          != NullId_T && downId      != NullId_T &&
			   rollLeftId    != NullId_T && rollRightId != NullId_T &&
			   speedToggleId != NullId_T && lookXId     != NullId_T &&
			   lookYId       != NullId_T && toggleId    != NullId_T &&
			   "devcamera input mappings changed");
	}

	// devcamera input handlers
	{
		using namespace input;

		int priority = 0;

		engine.inputSystem->registerCallback(priority, [&engine, &game, this](FrameMappedInput& mi){

			auto& scene = engine.sceneManager->getScene(game.sceneId);

			engine.inputSystem->handleInputAction(toggleId, mi, [this](MappedAction& ma, InputContext& c){
				// toggle this cam and the primary scene cam
				return true;
			});

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

			engine.inputSystem->handleInputAxis(lookXId, mi, [this](MappedAxis& ma, InputContext& c){
				/*SDL_Log("devcamera axis %s,  handled motion, relRaw=%d, relMapped=%0.1f",
						ma.inputMapping->name, ma.axisMotion->relRaw, ma.axisMotion->relMapped);*/
				yawRaw += ma.axisMotion->relRaw;
				yawMapped += ma.axisMotion->relMapped;
				return true;
			});

			engine.inputSystem->handleInputAxis(lookYId, mi, [this](MappedAxis& ma, InputContext& c){
				/*SDL_Log("devcamera axis %s,  handled motion, relRaw=%d, relMapped=%0.1f",
						ma.inputMapping->name, ma.axisMotion->relRaw, ma.axisMotion->relMapped);*/
				pitchRaw += ma.axisMotion->relRaw;
				pitchMapped += ma.axisMotion->relMapped;
				return true;
			});
		});
	}
}