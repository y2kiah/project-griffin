#ifndef GRIFFIN_TOOLS_H_
#define GRIFFIN_TOOLS_H_

#ifdef GRIFFIN_TOOLS_BUILD

#include <memory>
#include <future>
#include <script/ScriptManager_LuaJIT.h>


namespace griffin {
	// Forward declarations
	namespace resource {
		class ResourceLoader;
		typedef std::shared_ptr<ResourceLoader>	ResourceLoaderPtr;
		typedef std::weak_ptr<ResourceLoader>	ResourceLoaderWeakPtr;
	}

	namespace tools {
		
		using std::shared_ptr;
		using std::weak_ptr;
		using std::future;

		// Function declarations

		void setResourceLoaderPtr(const resource::ResourceLoaderPtr& resourcePtr);


		/**
		*
		*/
		class GriffinToolsManager {
		public:
			explicit GriffinToolsManager(Id_T toolsLuaStateId) :
				m_toolsLuaStateId(toolsLuaStateId)
			{}
			~GriffinToolsManager();

			void init(const script::ScriptManagerPtr& scriptPtr);

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