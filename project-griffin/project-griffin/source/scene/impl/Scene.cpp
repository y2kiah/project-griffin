/**
* @file Scene.cpp
* @author Jeff Kiah
*/
#include "../Scene.h"
#include <SDL_log.h>
#include <utility/memory_reserve.h>
#include <scene/Camera.h>
#include <entity/EntityManager.h>
#include <render/Render.h>
#include <glm/mat4x4.hpp>

using namespace griffin::scene;

griffin::render::RenderSystemWeakPtr g_renderPtr;

void griffin::scene::setRenderSystemPtr(const griffin::render::RenderSystemWeakPtr& renderPtr)
{
	g_renderPtr = renderPtr;
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
			s.sceneGraph->update();

			// update position/orientation of active camera from the scene graph
			//	only supports one active camera now, but the active cameras could be extended into a list to support several views
			for (auto& camInstance : s.entityManager->getComponentStore<CameraInstanceContainer>().getComponents()) {
				if (camInstance.component.cameraId == s.activeRenderCamera) {
					auto& cam = *s.cameras[camInstance.component.cameraId];
					SceneNodeId nodeId = s.entityManager->getEntityComponentId(camInstance.entityId, SceneNode::componentType);
					auto& node = s.entityManager->getComponentStore<SceneNode>().getComponent(nodeId);

					cam.setEyePoint(node.positionWorld);
					cam.setOrientation(node.orientationWorld);
					cam.calcModelView();

					break;
				}
			}
		}
	}
}

void SceneManager::renderActiveScenes()
{
	auto& render = *g_renderPtr.lock();

	for (auto& s : m_scenes.getItems()) {
		if (s.active) {
			// render all visible mesh instances

			
			// for each active camera, set the renderer viewport 
			auto& cam = *s.cameras[s.activeRenderCamera];
			
			glm::mat4 viewProjMat(cam.getProjectionMatrix() * cam.getModelViewMatrix());
			float frustumDistance = cam.getFarClip() - cam.getNearClip();

			render.setViewportParameters(s.activeRenderCamera, render::ViewportParameters{
				cam.getModelViewMatrix(),
				cam.getProjectionMatrix(),
				viewProjMat,
				cam.getNearClip(),
				cam.getFarClip(),
				frustumDistance,
				1.0f / frustumDistance
			});
		}
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
