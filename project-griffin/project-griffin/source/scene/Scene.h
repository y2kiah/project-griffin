/**
* @file Scene.h
* @author Jeff Kiah
*/
#ifndef GRIFFIN_SCENE_H_
#define GRIFFIN_SCENE_H_

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include "SceneGraph.h"
#include <entity/EntityManager.h>
#include <utility/memory_reserve.h>


namespace griffin {
	namespace scene {

		using namespace griffin::entity;


		class SceneManager;
		class Camera;

		typedef griffin::Id_T							SceneId;
		typedef std::shared_ptr<SceneManager>			SceneManagerPtr;
		typedef std::vector<std::shared_ptr<Camera>>	CameraList;

		
		// Function declarations

		extern void setSceneManagerPtr(const SceneManagerPtr& sceneMgrPtr);

		// Type declarations

		/**
		*
		*/
		class Scene {
		public:
			// Variables

			SceneGraph		sceneGraph;
			EntityManager	entityManager;
			CameraList		cameras;
			
			// contains Lua state?
			// contains layer id for RenderEntry???

			int32_t			activeRenderCamera = -1;

			bool			active = false;
			std::string		name;

			// Functions

			explicit Scene(const std::string& _name, bool _active);
			~Scene();
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