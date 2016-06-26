#include "TerrainSystem.h"
#include <render/Render.h>
#include <render/RenderResources.h>
#include <render/RenderHelpers.h>
#include <render/RenderTarget_GL.h>
#include <render/RenderTarget3D_GL.h>
#include <GL/glew.h>


void griffin::game::TerrainSystem::updateFrameTick(Game* pGame, Engine& engine, const UpdateInfo& ui)
{
	// this function could implement changing sky conditions
}


void griffin::game::TerrainSystem::renderFrameTick(Game* pGame, Engine& engine, float interpolation,
												   const int64_t realTime, const int64_t countsPassed)
{
}


void griffin::game::TerrainSystem::init(Game* pGame, const Engine& engine, const SDLApplication& app)
{
	using namespace griffin::render;
	using namespace resource;

	auto loader = g_resourceLoader.lock();
	if (!loader) {
		throw std::runtime_error("no resource loader");
	}

	auto terrainProg = loadShaderProgram(L"shaders/terrain.glsl", engine.renderSystem);

	//this->terrainProgram = loader->getResource(terrainProg).get();
}