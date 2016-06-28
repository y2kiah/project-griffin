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
	// move this rendering code to render callback, the frame tick function

}


void griffin::game::TerrainSystem::render(Id_T entityId, scene::Scene& scene, uint8_t viewport, Engine& engine)
{
}


void griffin::game::TerrainSystem::draw(Id_T entityId, int drawSetIndex)
{
	glBindVertexArray(vao);
	glPatchParameteri(GL_PATCH_VERTICES, 16);
	glDrawArrays(GL_PATCHES, 0, 16);
}


void griffin::game::TerrainSystem::init(Game* pGame, const Engine& engine, const SDLApplication& app)
{
	using namespace griffin::render;
	using namespace resource;

	auto terrainProg = loadShaderProgram(L"shaders/terrain.glsl", engine.renderSystem);
	terrainProgram = engine.resourceLoader->getResource(terrainProg).get();
	engine.resourceLoader->executeCallbacks();

	// temp
	for (int i = 0; i < 16; ++i) {
		tempHeight[i*3]   = static_cast<float>(i % 4) * 32.0f;
		tempHeight[i*3+1] = static_cast<float>(i / 4) * 32.0f;
		tempHeight[i*3+2] = (static_cast<float>(rand()) / RAND_MAX) * 32.0f;
	}
	
	vertexBuffer.loadFromMemory(reinterpret_cast<const unsigned char*>(tempHeight), sizeof(tempHeight));

	// build VAO for terrain
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	vertexBuffer.bind();
	glEnableVertexAttribArray(VertexLayout_Position);
	glVertexAttribPointer(VertexLayout_Position, 3, GL_FLOAT, GL_FALSE, 0, 0);
}


void griffin::game::TerrainSystem::deinit()
{
	if (vao != 0) {
		glDeleteVertexArrays(1, &vao);
	}
}