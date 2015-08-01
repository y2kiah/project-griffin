/**
* @file Scene.cpp
* @author Jeff Kiah
*/
#include "../Scene.h"
#include <SDL_log.h>
#include <utility/memory_reserve.h>
#include <scene/Camera.h>

using namespace griffin::scene;

// class Scene

Scene::Scene(const std::string& _name, bool _active) :
	entityManager{},
	sceneGraph(entityManager),
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
	m_scenes[sceneId].sceneGraph.setSceneId(sceneId);
	return sceneId;
}

SceneManager::SceneManager() :
	m_scenes(0, RESERVE_SCENEMANAGER_SCENES)
{}

SceneManager::~SceneManager() {
	if (m_scenes.capacity() > RESERVE_SCENEMANAGER_SCENES) {
		SDL_Log("check RESERVE_SCENEMANAGER_SCENES: original=%d, highest=%d", RESERVE_SCENEMANAGER_SCENES, m_scenes.capacity());
	}
}
