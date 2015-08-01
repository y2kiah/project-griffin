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
#include <application/applicationTypedefs.h>
#include <utility/container/handle_map.h>

namespace griffin {

	struct Engine {
		ThreadPoolPtr					threadPool		= nullptr;
		script::ScriptManagerPtr		scriptManager	= nullptr;
		input::InputSystemPtr			inputSystem		= nullptr;
		resource::ResourceLoaderPtr		resourceLoader	= nullptr;
		render::RenderSystemPtr			renderSystem	= nullptr;
		scene::SceneManagerPtr			sceneManager	= nullptr;

		Id_T							engineLuaState	= NullId_T;

		#ifdef GRIFFIN_TOOLS_BUILD
		tools::GriffinToolsManagerPtr	toolsManager	= nullptr;
		Id_T							toolsLuaState	= NullId_T;
		#endif
	};

	Engine make_engine(const SDLApplication& app);
	void destroy_engine(Engine& engine);
}

#endif