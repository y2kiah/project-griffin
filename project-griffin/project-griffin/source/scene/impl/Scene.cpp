/**
* @file Scene.cpp
* @author Jeff Kiah
*/
#include "../Scene.h"
#include <SDL_log.h>
#include <utility/memory_reserve.h>

using namespace griffin::scene;

// class SceneManager

SceneManager::SceneManager(const EntityManagerPtr& _entityMgrPtr) :
	m_scenes(0, RESERVE_SCENEMANAGER_SCENES),
	entityMgrPtr(entityMgrPtr)
{}

SceneManager::~SceneManager() {
	if (m_scenes.capacity() > RESERVE_SCENEMANAGER_SCENES) {
		SDL_Log("check RESERVE_SCENEMANAGER_SCENES: original=%d, highest=%d", RESERVE_SCENEMANAGER_SCENES, m_scenes.capacity());
	}
}
