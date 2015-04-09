#pragma once
#ifndef GRIFFIN_SCRIPTMANAGER_LUAJIT_H_
#define GRIFFIN_SCRIPTMANAGER_LUAJIT_H_

#include <memory>
#include <string>

typedef struct lua_State lua_State;

namespace griffin {
	namespace script {

		using std::shared_ptr;
		using std::string;


		class ScriptManager {
		public:
			explicit ScriptManager() {}
			~ScriptManager() {}

			bool init(const string &filename);
			void deinit();
			
			/**
			* Executes a string of Lua code.
			* @scriptStr null-terminated string
			* @returns true on success
			*/
			bool doString(const string &scriptStr);

			/**
			* Loads and executes Lua code from a file
			* @initScriptfilename file name of script to run
			* @returns true on success
			*/
			bool doFile(const string &initScriptfilename);

			/*template <typename Ret, typename ... Params>
			Ret callLuaGlobalFunction(const char* func, Params...) {
				lua_getglobal(m_state, func);

				return 
			}*/
			void callLuaGlobalFunction(const char* func); // TEMP

			void runUpdateScripts() const;
			void runFrameScripts() const;

		private:
			lua_State * m_state;

			ScriptManager(const ScriptManager&) = delete;
		};


		typedef shared_ptr<ScriptManager>	ScriptManagerPtr;

	}
}


#endif