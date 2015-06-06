#include "script/ScriptManager_LuaJIT.h"
#include <lua.hpp>
#include <SDL_log.h>
#include <utility/export.h>

#pragma comment ( lib, "lua51.lib" )

// temporary proof of concept using FFI for Lua binding
// MOVE debug_printf into a file in the API area
extern "C" {
	GRIFFIN_EXPORT
	void debug_printf(const char *fmt) {
		SDL_Log(fmt);
	}
}

namespace griffin {
	namespace script {

		ScriptManager::~ScriptManager()
		{
			for (lua_State* state : m_states.getItems()) {
				if (state != nullptr) {
					lua_close(state);
				}
			}
		}

		Id_T ScriptManager::createState(const string &initScriptfilename)
		{
			Id_T stateId = m_states.insert(luaL_newstate());
			lua_State* state = m_states[stateId];

			luaL_openlibs(state); // Load Lua libraries

			lua_newtable(state);
			lua_setglobal(state, "engine");

			// Load and run the global Lua initialization
			if (initScriptfilename.length() > 0) {
				doFile(state, initScriptfilename); // throws on error
			}

			return stateId;
		}

		void ScriptManager::destroyState(Id_T stateId)
		{
			assert(m_states.isValid(stateId) && "invalid Lua state");

			lua_State* state = m_states[stateId];
			if (state != nullptr) {
				lua_close(state);
			}
			m_states.erase(stateId);
		}


		int ScriptManager::doString(Id_T stateId, const string &scriptStr) const
		{
			assert(m_states.isValid(stateId) && "invalid Lua state");

			return doString(m_states[stateId], scriptStr);
		}

		int ScriptManager::doString(lua_State* state, const string &scriptStr) const
		{
			return luaL_dostring(state, scriptStr.c_str());
		}


		int ScriptManager::doFile(Id_T stateId, const string &filename, bool throwOnError)
		{
			assert(m_states.isValid(stateId) && "invalid Lua state");

			return doFile(m_states[stateId], filename, throwOnError);
		}

		int ScriptManager::doFile(lua_State* state, const string &filename, bool throwOnError)
		{
			int e = luaL_loadfile(state, filename.c_str());
			if (e == 0) {
				e = lua_pcall(state, 0, LUA_MULTRET, 0);
			}
			
			if (e == LUA_ERRRUN) {
				SDL_Log("ScriptManager: Lua runtime error: %s", lua_tostring(state, -1));
				if (throwOnError) {
					throw std::runtime_error("ScriptManager: Lua runtime error: " + string{ lua_tostring(state, -1) });
				}
			}
			else if (e == LUA_ERRSYNTAX) {
				SDL_Log("ScriptManager: Lua syntax error: %s", lua_tostring(state, -1));
				if (throwOnError) {
					throw std::runtime_error("ScriptManager: Lua syntax error: " + string{ lua_tostring(state, -1) });
				}
			}
			else if (e == LUA_ERRFILE) {
				SDL_Log("ScriptManager: couldn't load file: %s", lua_tostring(state, -1));
				if (throwOnError) {
					throw std::runtime_error("ScriptManager: couldn't load file: " + string{ lua_tostring(state, -1) });
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


		void ScriptManager::callLuaGlobalFunction(Id_T stateId, const char* func)
		{
			assert(m_states.isValid(stateId) && "invalid Lua state");

			callLuaGlobalFunction(m_states[stateId], func);
		}


		void ScriptManager::callLuaGlobalFunction(lua_State* state, const char* func)
		{
			lua_getglobal(state, func);
			if (lua_pcall(state, 0, 0, 0) != 0) {
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