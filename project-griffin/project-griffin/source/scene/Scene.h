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
	struct Engine;

	namespace render {
		class RenderSystem;
		typedef std::weak_ptr<RenderSystem> RenderSystemWeakPtr;
	}
	namespace resource {
		class ResourceLoader;
		typedef std::shared_ptr<ResourceLoader> ResourceLoaderPtr;
	}

	namespace scene {

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

		void setSceneManagerPtr(const SceneManagerPtr&);
		void setRenderSystemPtr(const render::RenderSystemWeakPtr&);
		void setResourceLoaderPtr(const resource::ResourceLoaderPtr&);


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
		* TODO: make multiple active cameras supported
		* This is the max number of active cameras for any one frame of a rendered scene. This
		* number includes cameras needed for rendering all viewports and shadow frustums. The
		* frustum culling results are stored in a 4-byte bitset, hence this limitation.
		*/
		#define SCENE_MAX_ACTIVE_CAMERAS	32

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

			void getVisibleEntities();

			uint32_t createCamera(const CameraParameters& cameraParams, bool makeActive = false);
			
			uint32_t getActiveCamera() const
			{
				return activeRenderCamera;
			}

			void setActiveCamera(uint32_t cameraId /*, TODO: take a viewport index */)
			{
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

			Scene& getScene(SceneId sceneId)
			{
				return m_scenes[sceneId];
			}

			SceneId createScene(const std::string& name, bool makeActive);

			void updateActiveScenes();
			void renderActiveScenes(float interpolation, Engine& engine);

		private:

			void frustumCullActiveScenes();

			// Private Variables

			handle_map<Scene> m_scenes;
		};

	}
}

#endif