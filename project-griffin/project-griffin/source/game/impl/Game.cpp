/**
* @file Game.cpp
* @author Jeff Kiah
*/

#include <game/Game.h>
#include <application/Engine.h>
#include <scene/Scene.h>
#include <entity/EntityManager.h>
#include <script/ScriptManager_LuaJIT.h>
#include <input/InputSystem.h>
#include <SDL_log.h>

#include <api/SceneApi.h>
#include <scene/Camera.h>

namespace griffin {
	
	enum {
		DevCameraMovementComponentTypeId = 0
	};

	struct DevCameraMovementComponent {
		float	moveForward;
		float	moveSide;
		float	moveVertical;
		bool	highSpeed;
	};


	/**
	* Create and init the initial game state and game systems and do dependency injection
	*/
	GamePtr make_game(const Engine& engine, const SDLApplication& app)
	{
		GamePtr gamePtr = std::make_unique<Game>();
		Game& game = *gamePtr.get();

		//entity::test_reflection(); // TEMP

		// InputSystem.lua contains initInputSystem function
		engine.scriptManager->doFile(engine.engineLuaState, "scripts/game/initGame.lua"); // throws on error

		// invoke Lua function to init the game
		engine.scriptManager->callLuaGlobalFunction(engine.engineLuaState, "initGame");

		// set up game scene
		{
			using namespace griffin::scene;

			game.sceneId = engine.sceneManager->createScene("Game World", true);
			auto& scene = engine.sceneManager->getScene(game.sceneId);

			// create game component stores for this scene
			// TEMP the devcamera store is not needed, only one of these things, just testing the waters
			game.gameComponentStoreIds[DevCameraMovementComponentTypeId] = scene.entityManager->createScriptComponentStore(
				DevCameraMovementComponentTypeId,
				sizeof(DevCameraMovementComponent), 1);

			// create dev camera system
			game.devCameraSystem.devCameraId = createCamera(game.sceneId, NullId_T, CameraParameters{
				0.1f, 100000.0f,	// near/far clip
				app.getPrimaryWindow().width, app.getPrimaryWindow().height, // viewport
				60.0f, Camera_Perspective
			}, "devcamera");
			
			// look at the origin
			auto camNode = scene.entityManager->getEntityComponentId(game.devCameraSystem.devCameraId, scene::SceneNode::componentType);
			auto& node = scene.entityManager->getComponentStore<scene::SceneNode>().getComponent(camNode);

			auto camInst = scene.entityManager->getEntityComponentId(game.devCameraSystem.devCameraId, scene::CameraInstanceContainer::componentType);
			auto& cam = scene.entityManager->getComponentStore<scene::CameraInstanceContainer>().getComponent(camInst);
			scene.cameras[cam.cameraId]->lookAt(glm::vec3{ 120.0f, 40.0f, 0 }, glm::vec3{ 0, 0, 0 }, glm::vec3{ 0, 1.0f, 0 });
			
			// set scene node location and orientation to the camera's
			node.translationLocal = scene.cameras[cam.cameraId]->getEyePoint();
			node.positionDirty = 1;
			node.rotationLocal = scene.cameras[cam.cameraId]->getOrientation();
			node.orientationDirty = 1;

			// add devcamera movement component
			game.devCameraSystem.devCameraMovementId = scene.entityManager->addScriptComponentToEntity(DevCameraMovementComponentTypeId,
																									   game.devCameraSystem.devCameraId);

			// get devcamera input mapping ids
			{
				auto ctx = engine.inputSystem->getInputContextHandle("devcamera");
				game.devCameraSystem.devCameraInputContextId = ctx;

				game.devCameraSystem.forward = engine.inputSystem->getInputMappingHandle("Move Forward", ctx);
				game.devCameraSystem.back = engine.inputSystem->getInputMappingHandle("Move Backward", ctx);
				game.devCameraSystem.left = engine.inputSystem->getInputMappingHandle("Move Left", ctx);
				game.devCameraSystem.right = engine.inputSystem->getInputMappingHandle("Move Right", ctx);
				game.devCameraSystem.up = engine.inputSystem->getInputMappingHandle("Move Up", ctx);
				game.devCameraSystem.down = engine.inputSystem->getInputMappingHandle("Move Down", ctx);
				game.devCameraSystem.highSpeed = engine.inputSystem->getInputMappingHandle("High Speed", ctx);
				game.devCameraSystem.lookX = engine.inputSystem->getInputMappingHandle("Mouse Look X", ctx);
				game.devCameraSystem.lookY = engine.inputSystem->getInputMappingHandle("Mouse Look Y", ctx);

				assert(game.devCameraSystem.forward != NullId_T && game.devCameraSystem.back != NullId_T &&
					   game.devCameraSystem.left != NullId_T    && game.devCameraSystem.right != NullId_T &&
					   game.devCameraSystem.up != NullId_T      && game.devCameraSystem.down != NullId_T &&
					   game.devCameraSystem.highSpeed != NullId_T && game.devCameraSystem.lookX != NullId_T &&
					   game.devCameraSystem.lookY != NullId_T && "devcamera input mappings changed");
			}
		}
		
		// devcamera input handlers
		{
			using namespace input;

			int priority = 0;

			engine.inputSystem->registerCallback(priority, [&engine, &game](FrameMappedInput& mi){

				auto& scene = engine.sceneManager->getScene(game.sceneId);
				auto devCamMove = (DevCameraMovementComponent*)scene.entityManager->getScriptComponentData(game.devCameraSystem.devCameraMovementId);

				engine.inputSystem->handleInputState(game.devCameraSystem.forward, mi, [devCamMove](MappedState& ms, InputContext& c){
					devCamMove->moveForward += 1.0f;
					return true;
				});

				engine.inputSystem->handleInputState(game.devCameraSystem.back, mi, [devCamMove](MappedState& ms, InputContext& c){
					devCamMove->moveForward -= 1.0f;
					return true;
				});

				engine.inputSystem->handleInputState(game.devCameraSystem.left, mi, [devCamMove](MappedState& ms, InputContext& c){
					devCamMove->moveSide -= 1.0f;
					return true;
				});

				engine.inputSystem->handleInputState(game.devCameraSystem.right, mi, [devCamMove](MappedState& ms, InputContext& c){
					devCamMove->moveSide += 1.0f;
					return true;
				});

				engine.inputSystem->handleInputState(game.devCameraSystem.up, mi, [devCamMove](MappedState& ms, InputContext& c){
					devCamMove->moveVertical += 1.0f;
					return true;
				});

				engine.inputSystem->handleInputState(game.devCameraSystem.down, mi, [devCamMove](MappedState& ms, InputContext& c){
					devCamMove->moveVertical -= 1.0f;
					return true;
				});

				engine.inputSystem->handleInputState(game.devCameraSystem.highSpeed, mi, [devCamMove](MappedState& ms, InputContext& c){
					devCamMove->highSpeed = true;
					return true;
				});

				engine.inputSystem->handleInputAxis(game.devCameraSystem.lookX, mi, [devCamMove](MappedAxis& ma, InputContext& c){
					SDL_Log("devcamera axis %s,  handled motion, relRaw=%d, relMapped=%0.1f",
							ma.inputMapping->name, ma.axisMotion->relRaw, ma.axisMotion->relMapped);

					return true;
				});

				engine.inputSystem->handleInputAxis(game.devCameraSystem.lookY, mi, [devCamMove](MappedAxis& ma, InputContext& c){
					SDL_Log("devcamera axis %s,  handled motion, relRaw=%d, relMapped=%0.1f",
							ma.inputMapping->name, ma.axisMotion->relRaw, ma.axisMotion->relMapped);

					return true;
				});

			});
		}

		// startup active input contexts
		engine.inputSystem->setContextActive(game.devCameraSystem.devCameraInputContextId);

		return std::move(gamePtr);
	}


	/**
	* Called to destroy the systems that should specifically be removed on the OpenGL thread
	*/
	void destroy_game(GamePtr& gamePtr)
	{
		
	}

}