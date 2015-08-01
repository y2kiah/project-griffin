#include <tools/GriffinTools.h>
#include <future>
#include <application/platform.h>
#include <SDL_log.h>


#ifdef GRIFFIN_TOOLS_BUILD

namespace griffin {
	namespace tools {

		// class GriffinTools

		GriffinToolsManager::~GriffinToolsManager()
		{
			m_done = true;
			m_toolsThread.wait();
		}


		void GriffinToolsManager::init(const script::ScriptManagerPtr& scriptPtr)
		{
			m_toolsThread = std::async(std::launch::async, [this, scriptPtr]() {
				

				// build lua files (resolve #includes, copy to data script path)
				scriptPtr->doFile(m_toolsLuaStateId, "scripts/luaBuild.lua");

				// start the tools http server
				scriptPtr->doFile(m_toolsLuaStateId, "scripts/tools/ToolsServer.lua");

				scriptPtr->callLuaGlobalFunction(m_toolsLuaStateId, "initToolsServer");
				while (!m_done) {
					try {
						scriptPtr->callLuaGlobalFunction(m_toolsLuaStateId, "frameToolsHandler");
					}
					catch (std::exception ex) {
						SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "tools server exception: %s", ex.what());
					}

					platform::yieldThread();
				}
			});
		}

	}
}

#endif