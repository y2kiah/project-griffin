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

		typedef griffin::Id_T    SceneId;


		/**
		*
		*/
		struct Scene {
			SceneGraph		sceneGraph;
			EntityManager	entityManager;
			// contains Cameras???
			// contains layer id for RenderEntry???
			std::string	name;
		};


		/**
		*
		*/
		class SceneManager {
		public:
			// Public Functions

			explicit SceneManager(const EntityManagerPtr& _entityMgrPtr);
			~SceneManager();

			Scene& getScene(SceneId sceneId) { return m_scenes[sceneId]; }
			
			SceneId createScene(const std::string& name);

			// Public Variables

			EntityManagerPtr entityMgrPtr;

		private:
			// Private Variables

			handle_map<Scene> m_scenes;
		};


		typedef std::shared_ptr<SceneManager> SceneManagerPtr;
	}
}

#endif