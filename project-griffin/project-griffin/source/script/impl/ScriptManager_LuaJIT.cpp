#include "script/ScriptManager_LuaJIT.h"
#include <lua.hpp>
#include <SDL_log.h>
#include <utility/export.h>

#pragma comment ( lib, "lua51.lib" )

// temporary proof of concept using FFI for Lua binding
extern "C" {
	GRIFFIN_EXPORT
	void debug_printf(const char *fmt) {
		SDL_Log(fmt);
	}
}

namespace griffin {
	namespace script {

		void ScriptManager::init(const string &initScriptfilename)
		{
			m_state = luaL_newstate();

			luaL_openlibs(m_state); // Load Lua libraries

			lua_newtable(m_state);
			lua_setglobal(m_state, "engine");

			// Load and run the global Lua initialization
			doFile(initScriptfilename); // throws on error
		}


		void ScriptManager::deinit()
		{
			lua_close(m_state);
		}


		int ScriptManager::doString(const string &scriptStr)
		{
			return luaL_dostring(m_state, scriptStr.c_str());
		}


		int ScriptManager::doFile(const string &filename, bool throwOnError)
		{
			int e = luaL_loadfile(m_state, filename.c_str());
			if (e == 0) {
				e = lua_pcall(m_state, 0, LUA_MULTRET, 0);
			}
			
			if (e == LUA_ERRRUN) {
				SDL_Log("ScriptManager: Lua runtime error: %s", lua_tostring(m_state, -1));
				if (throwOnError) {
					throw std::runtime_error("ScriptManager: Lua runtime error: " + string{ lua_tostring(m_state, -1) });
				}
			}
			else if (e == LUA_ERRSYNTAX) {
				SDL_Log("ScriptManager: Lua syntax error: %s", lua_tostring(m_state, -1));
				if (throwOnError) {
					throw std::runtime_error("ScriptManager: Lua syntax error: " + string{ lua_tostring(m_state, -1) });
				}
			}
			else if (e == LUA_ERRFILE) {
				SDL_Log("ScriptManager: couldn't load file: %s", lua_tostring(m_state, -1));
				if (throwOnError) {
					throw std::runtime_error("ScriptManager: couldn't load file: " + string{ lua_tostring(m_state, -1) });
				}
			}
			else if (e == LUA_ERRMEM) {
				SDL_Log("ScriptManager: Lua out of memory");
				if (throwOnError) {
					throw std::runtime_error("Lua out of memory");
				}
			}

			return e;
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