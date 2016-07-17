#include "TerrainSystem.h"
#include <render/Render.h>
#include <render/RenderResources.h>
#include <render/RenderHelpers.h>
#include <render/RenderTarget_GL.h>
#include <render/RenderTarget3D_GL.h>
#include <render/ShaderProgram_GL.h>
#include <render/texture/Texture2D_GL.h>
#include <GL/glew.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/matrix.hpp>
#include <utility/Debug.h>

// Local Variables

glm::mat4 bezierBasis(glm::vec4( 1.0f,-3.0f, 3.0f,-1.0f),
					  glm::vec4( 0.0f, 3.0f,-6.0f, 3.0f),
					  glm::vec4( 0.0f, 0.0f, 3.0f,-3.0f),
					  glm::vec4( 0.0f, 0.0f, 0.0f, 1.0f));

glm::mat4 bezierBasisTranspose(glm::transpose(bezierBasis));

// x(t)  = (h0)(1/6)(1-3t+3t^2-t^3) + (h1)(1/6)(4-6t^2+3t^3) + (h2)(1/6)(1+3t+3t^2-3t^3) + (h3)(1/6)(t^3)
glm::mat4 bicubicBasis(glm::vec4( 1.0f,-3.0f, 3.0f,-1.0f) / 6.0f,
					   glm::vec4( 4.0f, 0.0f,-6.0f, 3.0f) / 6.0f,
					   glm::vec4( 1.0f, 3.0f, 3.0f,-3.0f) / 6.0f,
					   glm::vec4( 0.0f, 0.0f, 0.0f, 1.0f) / 6.0f);

glm::mat4 bicubicBasisTranspose(glm::transpose(bicubicBasis));

glm::mat4 bicubicTangentBasis(glm::vec4(-3.0f,  6.0f,-3.0f, 0.0f) / 6.0f,
							  glm::vec4( 0.0f,-12.0f, 9.0f, 0.0f) / 6.0f,
							  glm::vec4( 3.0f,  6.0f,-9.0f, 0.0f) / 6.0f,
							  glm::vec4( 0.0f,  0.0f, 3.0f, 0.0f) / 6.0f);

glm::mat4 bicubicTangentBasisTranspose(glm::transpose(bicubicTangentBasis));

glm::mat4 catmullRomBasis(glm::vec4( 0.0f,-0.5f, 1.0f,-0.5f),
						  glm::vec4( 1.0f, 0.0f,-2.5f, 1.5f),
						  glm::vec4( 0.0f, 0.5f, 2.0f,-1.5f),
						  glm::vec4( 0.0f, 0.0f,-0.5f, 0.5f));

glm::mat4 catmullRomBasisTranspose(glm::transpose(catmullRomBasis));

// class TerrainSystem

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


void griffin::game::TerrainSystem::draw(Engine &engine, const glm::dmat4& viewMat, const glm::mat4& projMat/*All TEMP*/)
{
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	auto& renderSystem = *engine.renderSystem;

	auto& program = terrainProgram.get()->getResource<render::ShaderProgram_GL>();
	program.useProgram();

	// bind the patch heightmap texture
	auto& tex = tempNoiseTex->getResource<render::Texture2D_GL>();
	tex.bind(render::SamplerBinding_Diffuse1);

	// set the object UBO values
	render::ObjectUniformsUBO objectUniformsUBO{};
	glBindBuffer(GL_UNIFORM_BUFFER, renderSystem.getUBOHandle(render::ObjectUniforms));
	
	objectUniformsUBO.modelToWorld = glm::mat4();
	objectUniformsUBO.modelView = viewMat;
	objectUniformsUBO.modelViewProjection = projMat * glm::mat4(viewMat);
	objectUniformsUBO.normalMatrix = glm::mat4(glm::transpose(glm::inverse(glm::mat3(viewMat))));
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(render::ObjectUniformsUBO), &objectUniformsUBO);

	// set basis matrix uniforms
	//glUniformMatrix4fv(basisLoc, 1, GL_FALSE, &bicubicBasis[0][0]);
	//glUniformMatrix4fv(basisTransposeLoc, 1, GL_FALSE, &bicubicBasisTranspose[0][0]);

	// draw the patch
	glBindVertexArray(vao);
	glPatchParameteri(GL_PATCH_VERTICES, 16);
	glDrawElements(GL_PATCHES, (terrainX - 3) * (terrainY - 3) * 16, GL_UNSIGNED_SHORT, 0);

	ASSERT_GL_ERROR;
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}


void griffin::game::TerrainSystem::init(Game* pGame, const Engine& engine, const SDLApplication& app)
{
	using namespace griffin::render;
	using namespace resource;

	auto terrainProg = loadShaderProgram(L"shaders/terrain.glsl", engine.renderSystem);
	terrainProgram = engine.resourceLoader->getResource(terrainProg).get();
	engine.resourceLoader->executeCallbacks();

	auto& program = terrainProgram.get()->getResource<ShaderProgram_GL>();
	terrainProgramId = program.getProgramId();

	//basisLoc = glGetUniformLocation(terrainProgramId, "basis");
	//basisTransposeLoc = glGetUniformLocation(terrainProgramId, "basisTranspose");

	// temp
	for (int v = 0; v < (terrainX * terrainY); ++v) {
		tempHeight[v*3]   = static_cast<float>(v % terrainX) / terrainX;
		tempHeight[v*3+1] = static_cast<float>(v / terrainX) / terrainY;
		tempHeight[v*3+2] = (static_cast<float>(rand()) / RAND_MAX);
	}
	
	int i = 0;
	for (int yStart = 0; yStart < (terrainY - 3); ++yStart) {
		for (int xStart = 0; xStart < (terrainX - 3); ++xStart) {
			for (int yPatch = 0; yPatch < 4; ++yPatch) {
				for (int xPatch = 0; xPatch < 4; ++xPatch) {
					int y = yStart + yPatch;
					int x = xStart + xPatch;
					tempIndices[i] = y * terrainX + x;
					++i;
				}
			}
		}
	}

	// build VAO for terrain
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	vertexBuffer.loadFromMemory(reinterpret_cast<const unsigned char*>(tempHeight), sizeof(tempHeight));
	indexBuffer.loadFromMemory(reinterpret_cast<const unsigned char*>(tempIndices), sizeof(tempIndices), sizeof(uint16_t));

	glEnableVertexAttribArray(VertexLayout_Position);
	glVertexAttribPointer(VertexLayout_Position, 3, GL_FLOAT, GL_FALSE, 0, 0);
	
	glBindVertexArray(0);

	// TEMP, load noise texture
	tempNoiseTex = engine.resourceLoader->getResource(L"temp_noise", Cache_Materials);

	ASSERT_GL_ERROR;
}


void griffin::game::TerrainSystem::deinit()
{
	if (vao != 0) {
		glDeleteVertexArrays(1, &vao);
	}
}