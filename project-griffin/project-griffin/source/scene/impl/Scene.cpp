/**
* @file Scene.cpp
* @author Jeff Kiah
*/
#include "../Scene.h"
#include <SDL_log.h>
#include <utility/memory_reserve.h>

using namespace griffin::scene;

// class SceneManager

SceneId SceneManager::createScene(const std::string& name)
{
	auto sceneId = m_scenes.emplace(name);
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
