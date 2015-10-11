#include "SkySystem.h"
#include <render/RenderResources.h>


void griffin::game::SkySystem::updateFrameTick(Game* pGame, Engine& engine, const UpdateInfo& ui)
{
	// this function could implement changing sky conditions
}


void griffin::game::SkySystem::init(Game* pGame, const Engine& engine, const SDLApplication& app)
{
	using namespace render;
	using namespace resource;

	auto skyTex = loadTextureCubeMap(L"textures/skybox.dds", CacheType::Cache_Permanent, true);
	skyBoxCubeMap = engine.resourceLoader->getResource(skyTex).get();

}