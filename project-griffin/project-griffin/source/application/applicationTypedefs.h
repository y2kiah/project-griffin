/**
* @file applicationTypedefs.h
* @author Jeff Kiah
* Forward declarations for application layer header files
*/
#pragma once
#ifndef GRIFFIN_APPLICATIONTYPEDEFS_H_
#define GRIFFIN_APPLICATIONTYPEDEFS_H_

#include <memory>

namespace griffin {
	namespace script {
		class ScriptManager;
		typedef std::shared_ptr<ScriptManager>	ScriptManagerPtr;
	}
	namespace input {
		class InputSystem;
		typedef std::shared_ptr<InputSystem>	InputSystemPtr;
		typedef std::weak_ptr<InputSystem>		InputSystemWeakPtr;
	}
	namespace resource {
		class ResourceLoader;
		typedef std::shared_ptr<ResourceLoader>	ResourceLoaderPtr;
	}
	namespace render {
		class RenderSystem;
		typedef std::shared_ptr<RenderSystem>	RenderSystemPtr;
	}
	namespace scene {
		class SceneManager;
		typedef std::shared_ptr<SceneManager>	SceneManagerPtr;
	}
	namespace tools {
		class GriffinToolsManager;
		typedef std::shared_ptr<GriffinToolsManager> GriffinToolsManagerPtr;
	}
}

#endif