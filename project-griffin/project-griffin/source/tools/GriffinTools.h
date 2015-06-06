#ifndef GRIFFIN_TOOLS_H_
#define GRIFFIN_TOOLS_H_

#ifdef GRIFFIN_TOOLS_BUILD

#include <memory>
#include <future>
#include <script/ScriptManager_LuaJIT.h>
#include <core/CoreSystem.h>


namespace griffin {
	namespace tools {
		
		using std::shared_ptr;
		using std::future;
		using core::CoreSystem;

		/**
		*
		*/
		class GriffinToolsManager : public CoreSystem {
		public:
			explicit GriffinToolsManager(Id_T toolsLuaStateId) :
				m_toolsLuaStateId(toolsLuaStateId)
			{}
			~GriffinToolsManager();

			void init(const script::ScriptManagerPtr& scriptPtr);
			
			/**
			* Executed on the update thread
			*/
			virtual void update(const UpdateInfo& ui) override;

		private:
			Id_T			m_toolsLuaStateId;
			future<void>	m_toolsThread;
			bool			m_done = false;

		};

		typedef shared_ptr<GriffinToolsManager> GriffinToolsManagerPtr;

	}
}

#endif

#endif