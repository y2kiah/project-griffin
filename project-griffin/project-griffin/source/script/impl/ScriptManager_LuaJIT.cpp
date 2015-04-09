#include "script/ScriptManager_LuaJIT.h"
#include <lua.hpp>
#include <SDL_log.h>

#pragma comment ( lib, "lua51.lib" )

// temporary proof of concept using FFI for Lua binding
extern "C" {
	__declspec(dllexport) void debug_printf(const char *fmt) {
		SDL_Log(fmt);
	}
}

namespace griffin {
	namespace script {

		bool ScriptManager::init(const string &initScriptfilename)
		{
			m_state = luaL_newstate();

			luaL_openlibs(m_state); // Load Lua libraries

			lua_newtable(m_state);
			lua_setglobal(m_state, "engine");

			// Load the file containing the script we are going to run
			if (!doFile(initScriptfilename)) {
				return false;
			}

			return true;
		}


		void ScriptManager::deinit()
		{
			lua_close(m_state);
		}


		bool ScriptManager::doString(const string &scriptStr)
		{
			return (!luaL_dostring(m_state, scriptStr.c_str()));
		}


		bool ScriptManager::doFile(const string &filename)
		{
			int e = luaL_dofile(m_state, filename.c_str());
			if (e) {
				SDL_Log("ScriptManager: couldn't load file: %s\n", lua_tostring(m_state, -1));
			}
			return (!e);
		}


		void ScriptManager::callLuaGlobalFunction(const char* func)
		{
			lua_getglobal(m_state, func);
			if (lua_pcall(m_state, 0, 0, 0) != 0) {
			}
		}


		void ScriptManager::runUpdateScripts() const
		{
		}


		void ScriptManager::runFrameScripts() const
		{
		}
	}
}