/**
* @file Engine.h
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_ENGINE_H_
#define GRIFFIN_ENGINE_H_

#include <memory>
#include <vector>
#include <utility/concurrency.h>
#include <application/main.h>
#include <application/UpdateInfo.h>
#include <application/applicationTypedefs.h>
#include <utility/container/handle_map.h>

namespace griffin {
	struct Game;

	struct Engine {
		ThreadPoolPtr					threadPool		= nullptr;
		script::ScriptManagerPtr		scriptManager	= nullptr;
		input::InputSystemPtr			inputSystem		= nullptr;
		resource::ResourceLoaderPtr		resourceLoader	= nullptr;
		render::RenderSystemPtr			renderSystem	= nullptr;
		render::ShaderManagerPtr		shaderManager	= nullptr;
		//render::ModelManagerPtr			modelManager	= nullptr; // not sure I want this
		scene::SceneManagerPtr			sceneManager	= nullptr;

		Id_T							engineLuaState	= NullId_T;

		#ifdef GRIFFIN_TOOLS_BUILD
		tools::GriffinToolsManagerPtr	toolsManager	= nullptr;
		Id_T							toolsLuaState	= NullId_T;
		#endif
	};

	void engineUpdateFrameTick(Engine& engine, Game* pGgame, UpdateInfo& ui);
	void engineRenderFrameTick(Engine& engine, Game* pGame, float interpolation,
							   const int64_t realTime, const int64_t countsPassed);

	Engine make_engine(const SDLApplication& app);
	void destroy_engine(Engine& engine);
}

#endif