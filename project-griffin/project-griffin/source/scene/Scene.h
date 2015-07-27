/**
* @file Scene.h
* @author Jeff Kiah
*/
#ifndef GRIFFIN_SCENE_H_
#define GRIFFIN_SCENE_H_

#include <cstdint>
#include <string>
#include <memory>
#include "SceneGraph.h"
#include <entity/EntityManager.h>


namespace griffin {
	namespace scene {

		using namespace griffin::entity;

		// Forward declarations
		class SceneManager;

		// Typedefs/Enums

		typedef griffin::Id_T    SceneId;
		typedef std::shared_ptr<SceneManager> SceneManagerPtr;

		// Function declarations

		extern void setSceneManagerPtr(const SceneManagerPtr& sceneMgrPtr);

		// Type declarations

		/**
		*
		*/
		struct Scene {
			SceneGraph		sceneGraph;
			EntityManager	entityManager;
			
			// contains Cameras???
			// contains Lua state?
			// contains layer id for RenderEntry???
			bool			active = false;
			std::string		name;

			explicit Scene(const std::string& _name, bool _active) :
				entityManager{},
				sceneGraph(entityManager),
				name(_name),
				active{ _active }
			{}
		};


		/**
		*
		*/
		class SceneManager {
		public:
			// Public Functions

			explicit SceneManager();
			~SceneManager();

			Scene& getScene(SceneId sceneId) { return m_scenes[sceneId]; }
			
			SceneId createScene(const std::string& name, bool makeActive);

		private:
			// Private Variables

			handle_map<Scene> m_scenes;
		};

	}
}

#endif