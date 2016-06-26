#pragma once
#ifndef GRIFFIN_RENDER_RESOURCES_H_
#define GRIFFIN_RENDER_RESOURCES_H_

#include <string>
#include <memory>
#include <resource/Resource.h>
#include <render/texture/Texture2D_GL.h>
#include <render/texture/TextureCubeMap_GL.h>
#include <render/ShaderProgram_GL.h>
#include <render/model/Model_GL.h>

namespace griffin {
	using std::wstring;
	using std::shared_ptr;
	using std::weak_ptr;
	using resource::ResourceHandle;
	using resource::ResourcePtr;
	using resource::CacheType;

	namespace render {
		class RenderSystem;


		// Variables
		extern weak_ptr<resource::ResourceLoader> g_resourceLoader;

		// Resource Loading Functions
		ResourceHandle<Texture2D_GL>		loadTexture2D(wstring texturePath, CacheType cache = CacheType::Cache_Materials);

		ResourceHandle<TextureCubeMap_GL>	loadTextureCubeMap(wstring texturePath, bool swapY = false,
															   CacheType cache = CacheType::Cache_Materials);

		ResourceHandle<ShaderProgram_GL>	loadShaderProgram(wstring programPath, const shared_ptr<RenderSystem> &renderSystemPtr,
															  CacheType cache = CacheType::Cache_Materials);

		ResourceHandle<Model_GL>			loadModel(wstring modelFilePath, CacheType cache = CacheType::Cache_Models);

	}
}

#endif