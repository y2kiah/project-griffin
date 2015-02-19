#pragma once
#ifndef GRIFFIN_APPLICATION_H_
#define GRIFFIN_APPLICATION_H_

#include <memory>
#include "resource/ResourceLoader.h"
#include "script/ScriptManager_LuaJIT.h"


namespace griffin {

	struct Application {
		resource::ResourceLoaderPtr	resourceLoader = nullptr;
		script::ScriptManagerPtr	scriptManager = nullptr;
	};

	Application make_application();

}

#endif