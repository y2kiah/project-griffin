#include "DevConsoleSystem.h"
#include <scene/Scene.h>
#include <entity/EntityManager.h>
#include <input/InputSystem.h>
#include <game/impl/GameImpl.h>


void griffin::game::DevConsoleSystem::renderFrameTick(Game* pGame, Engine& engine, float interpolation,
													  const int64_t realTime, const int64_t countsPassed)
{
	Game& game = *pGame;
	

}


void griffin::game::DevConsoleSystem::init(Game* pGame, const Engine& engine, const SDLApplication& app)
{
	using namespace griffin::scene;
	Game& game = *pGame;
	auto& scene = engine.sceneManager->getScene(game.sceneId);

	// create game component stores for this system
	///// TEMP the devcamera store is not needed, only one of these things, just testing the waters
	//game.gameComponentStoreIds[DevCameraMovementComponentTypeId] = scene.entityManager->createScriptComponentStore(
	//	DevCameraMovementComponentTypeId,
	//	sizeof(DevCameraMovementComponent), 1);
	///// end TEMP

	//movementComponentId = scene.entityManager->getEntityComponentId(devCameraId, scene::MovementComponent::componentType);

	///// TEMP the devcamera store is not needed, demonstration
	// add devcamera movement component
	//devCameraMovementId = scene.entityManager->addScriptComponentToEntity(DevCameraMovementComponentTypeId,
	//																	  devCameraId);

	//auto devCamMove = (DevCameraMovementComponent*)scene.entityManager->getScriptComponentData(devCameraMovementId);
	///// end TEMP

	// get devconsole input mapping ids
	{
		auto ingameCtx = engine.inputSystem->getInputContextHandle("ingame");
		toggleId = engine.inputSystem->getInputMappingHandle("Show/Hide Dev Console", ingameCtx);

		auto ctx = engine.inputSystem->getInputContextHandle("devconsole");
		devConsoleInputContextId = ctx;

		assert(toggleId != NullId_T &&
			   "devconsole input mappings changed");
	}

	// devcamera input handlers
	{
		using namespace input;

		int priority = 0;

		engine.inputSystem->registerCallback(priority, [&engine, &game, this](FrameMappedInput& mi){

			//auto& scene = engine.sceneManager->getScene(game.sceneId);

			/**
			* Handle ingame context's devconsole toggle
			*/
			engine.inputSystem->handleInputAction(toggleId, mi, [this, &engine](MappedAction& ma, InputContext& c){
				visible = !visible;

				engine.inputSystem->setContextActive(devConsoleInputContextId, visible);
				if (visible) {
					wasRelativeMouseMode = engine.inputSystem->relativeMouseModeActive();
					engine.inputSystem->stopRelativeMouseMode();
				}
				else if (wasRelativeMouseMode) {
					engine.inputSystem->startRelativeMouseMode();
				}

				return true;
			});

			/**
			* Handle scroll with mousewheel
			*/
			/*engine.inputSystem->handleInputAction(speedScrollIncreaseId, mi, [this](MappedAction& ma, InputContext& c){
				
				return true;
			});

			engine.inputSystem->handleInputAction(speedScrollDecreaseId, mi, [this](MappedAction& ma, InputContext& c){
				
				return true;
			});*/

		});
	}
}
