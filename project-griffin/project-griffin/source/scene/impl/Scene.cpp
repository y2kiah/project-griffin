/**
* @file Scene.cpp
* @author Jeff Kiah
*/
#include "../Scene.h"
#include <application/Engine.h>
#include <resource/ResourceLoader.h>
#include <utility/memory_reserve.h>
#include <scene/Camera.h>
#include <entity/EntityManager.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <render/Render.h>
#include <render/geometry/Intersection.h>
#include <render/model/Model_GL.h>
#include <SDL_log.h>


using namespace griffin;
using namespace griffin::scene;


// Global Variables
render::RenderSystemWeakPtr g_renderPtr;
resource::ResourceLoaderWeakPtr g_resourceLoader;


// Forward Declarations
void interpolateSceneNodes(entity::EntityManager& entityMgr, float interpolation);


// Free functions

void scene::setRenderSystemPtr(const griffin::render::RenderSystemWeakPtr& renderPtr)
{
	g_renderPtr = renderPtr;
}

void scene::setResourceLoaderPtr(const resource::ResourceLoaderPtr& resourcePtr)
{
	g_resourceLoader = resourcePtr;
}


// class Scene

uint32_t Scene::createCamera(const CameraParameters& cameraParams, bool makeActive)
{
	CameraPtr camPtr = nullptr;

	if (cameraParams.cameraType == Camera_Perspective) {
		// add a perspective view camera
		camPtr = std::make_shared<CameraPersp>(cameraParams.viewportWidth, cameraParams.viewportHeight,
											   cameraParams.verticalFieldOfViewDegrees,
											   cameraParams.nearClipPlane, cameraParams.farClipPlane);
	}
	else if (cameraParams.cameraType == Camera_Ortho) {
		// add an orthographic view camera
		camPtr = std::make_shared<CameraOrtho>(0.0f, static_cast<float>(cameraParams.viewportWidth),
											   static_cast<float>(cameraParams.viewportHeight), 0.0f,
											   cameraParams.nearClipPlane, cameraParams.farClipPlane);
	}

	camPtr->calcMatrices();
	cameras.push_back(camPtr);
	uint32_t newCameraId = static_cast<uint32_t>(cameras.size() - 1);

	if (makeActive) {
		setActiveCamera(newCameraId);
	}

	return newCameraId;
}


Scene::Scene(const std::string& _name, bool _active) :
	entityManager(std::make_shared<EntityManager>()),
	sceneGraph(std::make_shared<SceneGraph>(*entityManager)),
	name(_name),
	active{ _active }
{
	cameras.reserve(RESERVE_SCENE_CAMERAS);
}

Scene::~Scene() {
	if (cameras.capacity() > RESERVE_SCENE_CAMERAS) {
		SDL_Log("check RESERVE_SCENE_CAMERAS: original=%d, highest=%d", RESERVE_SCENE_CAMERAS, cameras.capacity());
	}
}


// class SceneManager

SceneId SceneManager::createScene(const std::string& name, bool makeActive)
{
	auto sceneId = m_scenes.emplace(name, makeActive);
	m_scenes[sceneId].sceneGraph->setSceneId(sceneId);
	return sceneId;
}

void SceneManager::updateActiveScenes()
{
	for (auto& s : m_scenes.getItems()) {
		if (s.active) {
			// ???
		}
	}
}

void SceneManager::renderActiveScenes(float interpolation, Engine& engine)
{
	auto& render = *g_renderPtr.lock();
	auto& loader = *g_resourceLoader.lock();

	int8_t activeViewport = 0; // TEMP, hard coded to one viewport

	for (auto& s : m_scenes.getItems()) {
		if (s.active) {
			// run the movement system to interpolate all moving nodes in the scene
			interpolateSceneNodes(*s.entityManager, interpolation);
			
			// traverse scene graph, update world positions and orientations
			s.sceneGraph->updateNodeTransforms();

			// update position/orientation of active camera from the scene graph
			//	only supports one active camera now, but the active cameras could be extended into a list to support several views
			for (auto& camInstance : s.entityManager->getComponentStore<CameraInstance>().getComponents().getItems()) {
				if (camInstance.component.cameraId == s.activeRenderCamera) {
					auto& node = *s.entityManager->getEntityComponent<SceneNode>(camInstance.entityId);
					auto& cam = *s.cameras[s.activeRenderCamera];

					cam.setEyePoint(node.positionWorld);
					cam.setOrientation(node.orientationWorld);
					cam.calcModelView();

					// set the renderer viewport to the active camera
					mat4 viewProjMat(cam.getProjectionMatrix() * mat4(cam.getModelViewMatrix()));
					float frustumDistance = cam.getFarClip() - cam.getNearClip();

					// set viewport 0 which is the main view viewport
					render.setViewportParameters(activeViewport, render::ViewportParameters{
						cam.getModelViewMatrix(),
						cam.getProjectionMatrix(),
						viewProjMat,
						cam.getNearClip(),
						cam.getFarClip(),
						frustumDistance,
						1.0f / frustumDistance
					});

					break;
				}
			}

			// TODO: hard-coded to one frustum, need to support > 1 to get shadow mapping
			uint32_t activeFrustum = 0;
			uint32_t frustumMask = 1;

			// run the frustum culling system to determine visible objects
			frustumCullActiveScenes();

			// render all visible mesh instances
			auto& rciStore = s.entityManager->getComponentStore<RenderCullInfo>();
			
			// TODO: should we really loop through all components AGAIN? Frustum culling could build a list of entityids instead
			using namespace entity;
			for (auto& rci : rciStore.getComponents().getItems()) {
				// TODO: uncomment this once frustum culling is working
				//if (rci.component.visibleFrustumBits & frustumMask != 0) {
				
				ComponentMask mask = s.entityManager->getEntityComponentMask(rci.entityId);
				
				// if it's a Model_GL
				if (mask[ModelInstance::componentType]) {
					auto modelCmp = *s.entityManager->getEntityComponent<ModelInstance>(rci.entityId);
					auto modelPtr = loader.getResource(modelCmp.modelId, resource::Cache_Models);
					auto& model = modelPtr->getResource<render::Model_GL>();

					model.render(rci.entityId, s, activeViewport, engine);
				}
					//auto& node = *s.entityManager->getEntityComponent<SceneNode>(rci.entityId);
					
					// call "render" function which should only add render entries to the viewport's queue
					// the renderer will sort the queue and call the object's "draw" function with a callback function pointer
					/*
					RenderQueueKey key;
					key.value = rci.component.renderQueueKey;

					RenderEntry re{};
					re.entityId = rci.entityId;
					re.positionWorld = node.positionWorld;
					re.orientationWorld = node.orientationWorld;
					*/

					//render.addRenderEntry(activeViewport, key, std::move(re));
				//}
			}
		}
	}
}


/**
* 1.	After scene nodes are interpolated, update render entry worldspace AABB/quadtree indices.
* 2.	Filter objects "per camera/frustum" in wordspace using integer AABB/quadtree index
*		containing frustum against the integer AABB/quadtree index containing object.
*		Set frustum bit to 1 for each renderable object in each frustum space.
* 3.	Update filtered entity list viewspace positions and bounding spheres
* 4.	Frustum cull against view space bounding spheres, set frustum bits back to 0 if culled
* 5.	Final list of render entries is submitted to renderer. Double-precision world coordinates
*		are transformed to single-precision in viewspace.
*/
void SceneManager::frustumCullActiveScenes()
{
	using namespace geometry;

	for (auto& scene : m_scenes) {
		auto& rciStore = scene.entityManager->getComponentStore<RenderCullInfo>();

		//for (int activeFrustum = 0; activeFrustum < numActiveFrustums; frustumMask <<= 1; ++activeFrustum) {
			// To do the frustum index, we need an active cameras list, take index into that list for the camera.
			// Can have > 32 cameras in scene, but only up to 32 active cameras.
			// Do not simply take the camera index itself, we want the "active camera" index.
		uint32_t activeFrustum = 0;
		uint32_t frustumMask = 1;
		
			Plane frustumPlanes[6] = {};
			// TODO: code to get frustum

			for (auto& rci : rciStore.getComponents().getItems()) {
				auto inside = intersect(frustumPlanes, *reinterpret_cast<Sphere*>(&rci.component.viewspaceBSphere));
				rci.component.visibleFrustumBits |= frustumMask & (inside != Outside);
			}
		//}

		//for (i = 0; i < gridCell->blockCounts[blockIter]; ++i) {
			// filter list here (if masks[i] is zero it should be skipped)
			// ...
		//}
	}
}

SceneManager::SceneManager() :
	m_scenes(0, RESERVE_SCENEMANAGER_SCENES)
{}

SceneManager::~SceneManager() {
	if (m_scenes.capacity() > RESERVE_SCENEMANAGER_SCENES) {
		SDL_Log("check RESERVE_SCENEMANAGER_SCENES: original=%d, highest=%d", RESERVE_SCENEMANAGER_SCENES, m_scenes.capacity());
	}
}


// Free functions

void interpolateSceneNodes(entity::EntityManager& entityMgr, float interpolation)
{
	auto& moveComponents = entityMgr.getComponentStore<scene::MovementComponent>().getComponents();

	for (auto& move : moveComponents.getItems()) {
		if (move.component.rotationDirty == 0 && move.component.translationDirty == 0) {
			continue;
		}

		auto pNode = entityMgr.getEntityComponent<scene::SceneNode>(move.entityId);
		assert(pNode != nullptr);
		auto& node = *pNode;

		if (move.component.rotationDirty == 1) {
			node.rotationLocal = glm::slerp(move.component.prevRotation,
											move.component.nextRotation,
											static_cast<double>(interpolation));
			node.orientationDirty = 1;
		}

		if (move.component.translationDirty == 1) {
			node.translationLocal = glm::mix(move.component.prevTranslation,
											 move.component.nextTranslation,
											 static_cast<double>(interpolation));
			node.positionDirty = 1;
		}
	}
}
