#pragma once
#ifndef GRIFFIN_GAME_TERRAINSYSTEM_H_
#define GRIFFIN_GAME_TERRAINSYSTEM_H_

#include <game/Game.h>
#include <application/Engine.h>
#include <application/UpdateInfo.h>
#include <resource/ResourceLoader.h>
#include "render/VertexBuffer_GL.h"
#include "render/IndexBuffer_GL.h"

#include <glm/mat4x4.hpp> // TEMP
#include <glm/vec3.hpp>

namespace griffin {
	using resource::ResourcePtr;
	using resource::CacheType;
	using render::VertexBuffer_GL;
	using render::IndexBuffer_GL;

	namespace scene {
		class Scene;
	}

	namespace game {

		struct TerrainSystem {
			// Variables
			
			static const int patchSize = 32;

			ResourcePtr terrainProgram = nullptr;		//<! terrain shader program
			ResourcePtr tempNoiseTex = nullptr;

			ResourcePtr tempGrassTex = nullptr;
			ResourcePtr tempRockTex = nullptr;

			VertexBuffer_GL	vertexBuffer;
			IndexBuffer_GL	indexBuffer;
			unsigned int vao = 0;
			unsigned int terrainProgramId = 0;
			//int basisLoc = 0;
			//int basisTransposeLoc = 0;
//			int patchTopLeftCoordLoc = 0;
//			int patchCubeNormalLoc = 0;
			int patchLengthLoc = 0;
			int patchToModelLoc = 0;

			struct TerrainChunk {
				glm::dvec3  cubeFaceScale;
				glm::dvec3  geocentricCenterCoord;
				double      length;
				uint32_t    zOrder;
				uint8_t     level;

				ResourcePtr heightNoise;
			};

			TerrainChunk chunks[6];


			// Public Functions

			void updateFrameTick(Game& game, Engine& engine, const UpdateInfo& ui);
			
			void renderFrameTick(Game& game, Engine& engine, float interpolation,
								 const int64_t realTime, const int64_t countsPassed);

			void render(Id_T entityId, scene::Scene& scene, uint8_t viewport, Engine& engine);
			void draw(Engine &engine, const glm::dmat4& viewMat, const glm::mat4& projMat/*All TEMP*/);

			void init(Game& game, const Engine& engine, const SDLApplication& app);
			void deinit();
		};

	}
}

#endif