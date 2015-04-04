#pragma once
#ifndef GRIFFIN_APPLICATION_H_
#define GRIFFIN_APPLICATION_H_

#include <memory>
#include <vector>
#include <core/InputSystem.h>
#include <resource/ResourceLoader.h>
#include <script/ScriptManager_LuaJIT.h>
#include <entity/Entity.h>


namespace griffin {

	struct Application {
		script::ScriptManagerPtr		scriptManager	= nullptr;
		core::InputSystemPtr			inputSystem		= nullptr;
		resource::ResourceLoaderPtr		resourceLoader	= nullptr;
		entity::EntityManagerPtr		entityManager	= nullptr;

		std::vector<core::CoreSystem*>	systems;
	};

	Application make_application();

}

#endif