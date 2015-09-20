#pragma once
#ifndef GRIFFIN_RENDER_RESOURCES_H_
#define GRIFFIN_RENDER_RESOURCES_H_

#include <string>
#include <resource/Resource.h>
#include <render/texture/Texture2D_GL.h>
#include <render/ShaderProgram_GL.h>
#include <render/model/Model_GL.h>

namespace griffin {
	using std::wstring;
	using resource::ResourceHandle;
	using resource::ResourcePtr;
	using resource::CacheType;

	namespace render {

		// Resource Loading Functions
		ResourceHandle<Texture2D_GL>		loadTexture(wstring texturePath, CacheType cache = CacheType::Cache_Materials_T);

		ResourceHandle<ShaderProgram_GL>	loadShaderProgram(wstring programPath, CacheType cache = CacheType::Cache_Materials_T);

		ResourceHandle<Model_GL>			loadModel(wstring modelFilePath, CacheType cache = CacheType::Cache_Models_T);

	}
}

#endif