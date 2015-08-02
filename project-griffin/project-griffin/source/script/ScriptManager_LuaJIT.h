/**
* @file ScriptManager_LuaJIT
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_SCRIPTMANAGER_LUAJIT_H_
#define GRIFFIN_SCRIPTMANAGER_LUAJIT_H_

#include <memory>
#include <string>
#include <utility/container/handle_map.h>
#include <utility/memory_reserve.h>


typedef struct lua_State lua_State;

namespace griffin {
	namespace script {

		using std::shared_ptr;
		using std::string;


		// TODO: compute a CRC of each script delivered and do a simple check at runtime to see if it has been modified

		class ScriptManager {
		public:
			explicit ScriptManager() :
				m_states(0, RESERVE_LUA_STATES)
			{}
			~ScriptManager();

			Id_T createState(const string &initScriptfilename = "");
			
			void destroyState(Id_T stateId);

			/**
			* Executes a string of Lua code.
			* @scriptStr null-terminated string
			* @returns Lua error code (0 = no errors, LUA_ERRSYNTAX, LUA_ERRMEM, LUA_ERRFILE)
			*/
			int doString(Id_T stateId, const string &scriptStr) const;

			/**
			* Loads and executes Lua code from a file
			* @initScriptfilename file name of script to run
			* @returns Lua error code (0 = no errors, LUA_ERRSYNTAX, LUA_ERRMEM, LUA_ERRFILE)
			*/
			int doFile(Id_T stateId, const string &initScriptfilename, bool throwOnError = true);

			/*template <typename Ret, typename ... Params>
			Ret callLuaGlobalFunction(const char* func, Params...) {
				lua_getglobal(m_state, func);

				return 
			}*/
			int callLuaGlobalFunction(Id_T stateId, const char* func, bool throwOnError = true); // TEMP?

			void runUpdateScripts() const;
			void runFrameScripts() const;

		private:
			int doString(lua_State* state, const string &scriptStr) const;
			int doFile(lua_State* state, const string &filename, bool throwOnError = true);
			int callLuaGlobalFunction(lua_State* state, const char* func, bool throwOnError = true); // TEMP?


			// Variables

			handle_map<lua_State*>	m_states;

			ScriptManager(const ScriptManager&) = delete;
		};


		typedef shared_ptr<ScriptManager>	ScriptManagerPtr;

	}
}


#endif