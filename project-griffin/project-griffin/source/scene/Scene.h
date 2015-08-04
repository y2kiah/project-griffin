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
#include <utility/memory_reserve.h>


namespace griffin {
	namespace scene {

		//using namespace griffin::entity;

		class SceneManager;
		class Camera;
		class entity::EntityManager;

		typedef griffin::Id_T					SceneId;
		typedef std::shared_ptr<SceneManager>	SceneManagerPtr;
		typedef std::shared_ptr<EntityManager>	EntityManagerPtr;
		typedef std::shared_ptr<SceneGraph>		SceneGraphPtr;
		typedef std::shared_ptr<Camera>			CameraPtr;
		typedef std::vector<CameraPtr>			CameraList;

		
		// Function declarations

		extern void setSceneManagerPtr(const SceneManagerPtr& sceneMgrPtr);

		// Type declarations
		enum CameraFlags : uint8_t {
			Camera_Perspective	= 0,
			Camera_Ortho		= 1
		};

		struct CameraParameters {
			float		nearClipPlane;
			float		farClipPlane;
			uint32_t	viewportWidth;
			uint32_t	viewportHeight;
			float		verticalFieldOfViewDegrees;
			uint8_t		cameraType;
			uint8_t		_padding_end[3];
		};

		/**
		*
		*/
		class Scene {
		public:
			// Variables

			EntityManagerPtr	entityManager;
			SceneGraphPtr		sceneGraph;
			CameraList			cameras;
			
			// contains Lua state?
			// contains layer id for RenderEntry???

			int32_t				activeRenderCamera = -1;
			bool				active = false;
			std::string			name;

			// Functions

			uint32_t createCamera(const CameraParameters& cameraParams, bool makeActive = false);
			
			void setActiveCamera(uint32_t cameraId) {
				assert(cameraId >= 0 && cameraId < cameras.size() && "camera id out of range");
				activeRenderCamera = cameraId;
			}

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

			void updateActiveScenes();

		private:
			// Private Variables

			handle_map<Scene> m_scenes;
		};

	}
}

#endif