#pragma once
#ifndef GRIFFIN_GAME_TERRAINSYSTEM_H_
#define GRIFFIN_GAME_TERRAINSYSTEM_H_

#include <game/Game.h>
#include <application/Engine.h>
#include <application/UpdateInfo.h>
#include <resource/ResourceLoader.h>
#include "render/VertexBuffer_GL.h"

namespace griffin {
	using resource::ResourcePtr;
	using resource::CacheType;
	using render::VertexBuffer_GL;

	namespace scene {
		class Scene;
	}

	namespace game {

		struct TerrainSystem {
			// Variables
			
			float tempHeight[16*3] = {};

			ResourcePtr terrainProgram = nullptr;		//<! terrain shader program

			VertexBuffer_GL	vertexBuffer;
			unsigned int vao = 0;

			// Public Functions

			void updateFrameTick(Game* pGame, Engine& engine, const UpdateInfo& ui);
			
			void renderFrameTick(Game* pGame, Engine& engine, float interpolation,
								 const int64_t realTime, const int64_t countsPassed);

			void render(Id_T entityId, scene::Scene& scene, uint8_t viewport, Engine& engine);
			void draw(Id_T entityId, int drawSetIndex);

			void init(Game* pGame, const Engine& engine, const SDLApplication& app);
			void deinit();
		};

	}
}

#endif