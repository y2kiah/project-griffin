#include "DevConsoleSystem.h"
#include <scene/Scene.h>
#include <entity/EntityManager.h>
#include <input/InputSystem.h>
#include <game/impl/GameImpl.h>


void griffin::game::DevConsoleSystem::renderFrameTick(Game& game, Engine& engine, float interpolation,
													  const int64_t realTime, const int64_t countsPassed)
{
	
}


void griffin::game::DevConsoleSystem::init(Game& game, const Engine& engine, const SDLApplication& app)
{
	using namespace griffin::scene;
	auto& scene = engine.sceneManager->getScene(game.sceneId);

	// get devconsole input mapping ids
	{
		auto ingameCtx = engine.inputSystem->getInputContextHandle("ingame");
		toggleId = engine.inputSystem->getInputMappingHandle("Show/Hide Dev Console", ingameCtx);

		auto ctx = engine.inputSystem->getInputContextHandle("devconsole");
		devConsoleInputContextId = ctx;

		assert(toggleId != NullId_T &&
			   "devconsole input mappings changed");
	}

	// devconsole input handlers
	{
		using namespace input;

		// set the callback priority to the same as input context priority
		int priority = engine.inputSystem->getContext(devConsoleInputContextId).priority;

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
			* Handle text input
			* TODO: should instead pass this event along to a resusable GUI textbox handler
			*/
			engine.inputSystem->handleTextInput(devConsoleInputContextId, mi, [this](FrameMappedInput& mi, InputContext& c){
				// use mi.textInput
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
