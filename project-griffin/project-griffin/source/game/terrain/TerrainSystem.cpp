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
#include <glm/gtc/matrix_transform.hpp>
#include <utility/debug.h>

using namespace glm;

// Local Variables

mat4 bezierBasis(vec4( 1.0f,-3.0f, 3.0f,-1.0f),
				 vec4( 0.0f, 3.0f,-6.0f, 3.0f),
				 vec4( 0.0f, 0.0f, 3.0f,-3.0f),
				 vec4( 0.0f, 0.0f, 0.0f, 1.0f));

mat4 bezierBasisTranspose(transpose(bezierBasis));

// x(t)  = (h0)(1/6)(1-3t+3t^2-t^3) + (h1)(1/6)(4-6t^2+3t^3) + (h2)(1/6)(1+3t+3t^2-3t^3) + (h3)(1/6)(t^3)
mat4 bicubicBasis(vec4( 1.0f,-3.0f, 3.0f,-1.0f) / 6.0f,
				  vec4( 4.0f, 0.0f,-6.0f, 3.0f) / 6.0f,
				  vec4( 1.0f, 3.0f, 3.0f,-3.0f) / 6.0f,
				  vec4( 0.0f, 0.0f, 0.0f, 1.0f) / 6.0f);

mat4 bicubicBasisTranspose(transpose(bicubicBasis));

mat4 bicubicTangentBasis(vec4(-3.0f,  6.0f,-3.0f, 0.0f) / 6.0f,
						 vec4( 0.0f,-12.0f, 9.0f, 0.0f) / 6.0f,
						 vec4( 3.0f,  6.0f,-9.0f, 0.0f) / 6.0f,
						 vec4( 0.0f,  0.0f, 3.0f, 0.0f) / 6.0f);

mat4 bicubicTangentBasisTranspose(transpose(bicubicTangentBasis));

mat4 catmullRomBasis(vec4( 0.0f,-0.5f, 1.0f,-0.5f),
					 vec4( 1.0f, 0.0f,-2.5f, 1.5f),
					 vec4( 0.0f, 0.5f, 2.0f,-1.5f),
					 vec4( 0.0f, 0.0f,-0.5f, 0.5f));

mat4 catmullRomBasisTranspose(transpose(catmullRomBasis));

// class TerrainSystem

void griffin::game::TerrainSystem::updateFrameTick(Game& game, Engine& engine, const UpdateInfo& ui)
{
	// this function could implement changing sky conditions
}


void griffin::game::TerrainSystem::renderFrameTick(Game& game, Engine& engine, float interpolation,
												   const int64_t realTime, const int64_t countsPassed)
{
	// move this rendering code to render callback, the frame tick function

}


void griffin::game::TerrainSystem::render(Id_T entityId, scene::Scene& scene, uint8_t viewport, Engine& engine)
{
}


void griffin::game::TerrainSystem::draw(Engine &engine, const dmat4& viewMat, const mat4& projMat/*All TEMP*/)
{
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	auto& renderSystem = *engine.renderSystem;

	auto& program = terrainProgram.get()->getResource<render::ShaderProgram_GL>();
	program.useProgram();

	for (int c = 0; c < 6; ++c) { // for each chunk
		dmat4 modelToWorld;
		mat4 patchToModel;

		// temp
		if (c == 0) { // top
			//modelToWorld = translate(modelToWorld, dvec3{ -2500.0, -2500.0, 0.0 });
		}
		else if (c == 1) { // left
			patchToModel = rotate(patchToModel,
								  pi<float>() * 0.5f,
								  vec3{ 1.0f, 0.0f, 0.0f });
		}
		else if (c == 2) { // bottom
			patchToModel = rotate(patchToModel,
								  pi<float>(),
								  vec3{ 1.0f, 0.0f, 0.0f });
		}
		else if (c == 3) { // right
			patchToModel = rotate(patchToModel,
								  pi<float>() * 1.5f,
								  vec3{ 1.0f, 0.0f, 0.0f });
		}
		else if (c == 4) {
			patchToModel = rotate(patchToModel,
								  pi<float>() * 0.5f,
								  vec3{ 0.0f, 1.0f, 0.0f });
		}
		else if (c == 5) {
			patchToModel = rotate(patchToModel,
								  pi<float>() * 1.5f,
								  vec3{ 0.0f, 1.0f, 0.0f });
		}
		
		dmat4 modelView_World(viewMat * modelToWorld);

		dvec4 nodeTranslationWorld(modelToWorld[0][3], modelToWorld[1][3], modelToWorld[2][3], 1.0);
		vec3 nodeTranslation_Camera(nodeTranslationWorld * modelView_World);

		mat4 modelView_Camera(modelView_World);
		modelView_Camera[0][3] = nodeTranslation_Camera.x;
		modelView_Camera[1][3] = nodeTranslation_Camera.y;
		modelView_Camera[2][3] = nodeTranslation_Camera.z;

		mat4 mvp(projMat * modelView_Camera);
		mat4 normalMat(transpose(inverse(mat3(modelView_Camera))));


		//glm::vec3 chunkTopLeftPosCameraSpace = chunks[c].cubeFaceScale;
		//glUniform3fv(patchTopLeftCoordLoc, 1, &chunkTopLeftPosCameraSpace[0]);
		glUniform1f(patchLengthLoc, 11400000.0f);
		glUniformMatrix4fv(patchToModelLoc, 1, GL_FALSE, &patchToModel[0][0]);

		// bind the patch heightmap texture
		auto& heightTex = tempNoiseTex->getResource<render::Texture2D_GL>();
		heightTex.bind(render::SamplerBinding_Diffuse1);

		// bind the rock and grass textures
		auto& rockTex = tempRockTex->getResource<render::Texture2D_GL>();
		rockTex.bind(render::SamplerBinding_Diffuse2);
		auto& grassTex = tempGrassTex->getResource<render::Texture2D_GL>();
		grassTex.bind(render::SamplerBinding_Diffuse3);

		// set the object UBO values
		render::ObjectUniformsUBO objectUniformsUBO{};
		glBindBuffer(GL_UNIFORM_BUFFER, renderSystem.getUBOHandle(render::ObjectUniforms));

		objectUniformsUBO.modelToWorld = mat4(modelToWorld);
		objectUniformsUBO.modelView = modelView_Camera;
		objectUniformsUBO.modelViewProjection = mvp;
		objectUniformsUBO.normalMatrix = normalMat;
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(render::ObjectUniformsUBO), &objectUniformsUBO);

		// set basis matrix uniforms
		//glUniformMatrix4fv(basisLoc, 1, GL_FALSE, &bicubicBasis[0][0]);
		//glUniformMatrix4fv(basisTransposeLoc, 1, GL_FALSE, &bicubicBasisTranspose[0][0]);

		// draw the patch
		glBindVertexArray(vao);
		glPatchParameteri(GL_PATCH_VERTICES, 16);
		glDrawElements(GL_PATCHES, (patchSize - 3) * (patchSize - 3) * 16, GL_UNSIGNED_SHORT, 0);

	}

	ASSERT_GL_ERROR;
	
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}


void griffin::game::TerrainSystem::init(Game& game, const Engine& engine, const SDLApplication& app)
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
//	patchTopLeftCoordLoc = glGetUniformLocation(terrainProgramId, "patchTopLeftCoord");
//	patchCubeNormalLoc = glGetUniformLocation(terrainProgramId, "patchCubeNormal");
	patchLengthLoc = glGetUniformLocation(terrainProgramId, "patchLength");
	patchToModelLoc = glGetUniformLocation(terrainProgramId, "patchToModel");

	float vertices[patchSize * patchSize * 2] = {};
	uint16_t indices[(patchSize - 3)*(patchSize - 3) * 16] = {};

	int visiblePatchSize = patchSize - 3;

	for (int v = 0; v < (patchSize * patchSize); ++v) {
		vertices[v * 2]     = (static_cast<float>(v % patchSize) - ((patchSize - 1) * 0.5f)) / (patchSize - 3);
		vertices[v * 2 + 1] = (static_cast<float>(v / patchSize) - ((patchSize - 1) * 0.5f)) / (patchSize - 3);
	}
	
	int i = 0;
	for (int yStart = 0; yStart < (patchSize - 3); ++yStart) {
		for (int xStart = 0; xStart < (patchSize - 3); ++xStart) {
			for (int yPatch = 0; yPatch < 4; ++yPatch) {
				for (int xPatch = 0; xPatch < 4; ++xPatch) {
					int y = yStart + yPatch;
					int x = xStart + xPatch;
					indices[i] = y * patchSize + x;
					++i;
				}
			}
		}
	}

	// build VAO for terrain
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	vertexBuffer.loadFromMemory(reinterpret_cast<const unsigned char*>(vertices), sizeof(vertices));
	indexBuffer.loadFromMemory(reinterpret_cast<const unsigned char*>(indices), sizeof(indices), sizeof(uint16_t));

	glEnableVertexAttribArray(VertexLayout_Position);
	glVertexAttribPointer(VertexLayout_Position, 2, GL_FLOAT, GL_FALSE, 0, 0);
	
	glBindVertexArray(0);

	// TEMP, load noise texture
	tempNoiseTex = engine.resourceLoader->getResource(L"temp_noise", Cache_Materials);

	// TEMP, load rock and grass textures
	auto tmpRock  = loadTexture2D(L"assets/textures/terrain/rock002/rock002_diffuse_height.dds");
	auto tmpGrass = loadTexture2D(L"assets/textures/terrain/grass002/grass002_diffuse_height.dds");

	tempRockTex  = engine.resourceLoader->getResource(tmpRock).get();
	tempGrassTex = engine.resourceLoader->getResource(tmpGrass).get();
	
	engine.resourceLoader->executeCallbacks();

	// terrain chunks

	const double radius = 5000.0;
	chunks[0] = { {  1.0,  1.0,  1.0 }, {  0.0,  0.0,  1.0 }, radius*2, 0, 0, tempNoiseTex }; // top
	chunks[1] = { { -1.0, -1.0, -1.0 }, {  0.0,  0.0, -1.0 }, radius*2, 0, 0, tempNoiseTex }; // bottom

	chunks[2] = { { -radius, -radius,  radius }, {  0.0, -1.0,  0.0 }, radius*2, 0, 0, tempNoiseTex }; // front
	chunks[3] = { { -radius,  radius, -radius }, {  0.0,  1.0,  0.0 }, radius*2, 0, 0, tempNoiseTex }; // back

	chunks[4] = { { -radius,  radius,  radius }, { -1.0,  0.0,  0.0 }, radius*2, 0, 0, tempNoiseTex }; // left
	chunks[5] = { {  radius, -radius,  radius }, {  1.0,  0.0,  0.0 }, radius*2, 0, 0, tempNoiseTex }; // right

	ASSERT_GL_ERROR;
}


void griffin::game::TerrainSystem::deinit()
{
	if (vao != 0) {
		glDeleteVertexArrays(1, &vao);
	}
}


/* Notes

Need to have:
- center of planet (0,0,0)
- baseline radius of planet
- geocentric vector to camera
- distance from center to camera
- avg. distance from center to patch 
- six quadtrees
- 

*/