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
#include <core/InputSystem.h>
#include <resource/ResourceLoader.h>
#include <script/ScriptManager_LuaJIT.h>
#include <render/Render.h>
#include <scene/Scene.h>
#include <tools/GriffinTools.h>


namespace griffin {

	struct Engine {
		ThreadPoolPtr					threadPool		= nullptr;
		script::ScriptManagerPtr		scriptManager	= nullptr;
		core::InputSystemPtr			inputSystem		= nullptr;
		resource::ResourceLoaderPtr		resourceLoader	= nullptr;
		render::RenderSystemPtr			renderSystem	= nullptr;
		scene::SceneManagerPtr			sceneManager	= nullptr;

		std::vector<core::CoreSystem*>	systems;

		#ifdef GRIFFIN_TOOLS_BUILD
		tools::GriffinToolsManagerPtr	toolsManager	= nullptr;
		#endif
	};

	Engine make_engine(const SDLApplication& app);
	void destroy_engine(Engine& engine);
}

#endif