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
			std::unique_ptr<SceneGraph>		sceneGraph;
			std::unique_ptr<EntityManager>	entityManager;
			// contains Cameras???
			// contains layer id for RenderEntry???
			std::string	name;

			explicit Scene(Scene&& s) {}
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
			
			SceneId createScene(const std::string& name) {
				auto sceneId = m_scenes.insert({
					std::make_unique<SceneGraph>(*entityMgrPtr),	// sceneGraph
					std::make_unique<EntityManager>(),				// entityManager
					name											// name
				});
				m_scenes[sceneId].sceneGraph->setSceneId(sceneId);
				return sceneId;
			}

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