/**
* @file GriffinToolsApi.cpp
* @author Jeff Kiah
*/
#include <api/GriffinToolsApi.h>
#include <tools/GriffinTools.h>
#include <resource/ResourceLoader.h>
#include <render/model/Mesh_GL.h>
#include <render/model/ModelImport_Assimp.h>
#include <utility/concurrency.h>
#include <SDL_log.h>
#include <fstream>


griffin::resource::ResourceLoaderWeakPtr g_resourceLoader;

void griffin::tools::setResourceLoaderPtr(const resource::ResourceLoaderPtr& resourcePtr)
{
	g_resourceLoader = resourcePtr;
}


#ifdef __cplusplus
extern "C" {
#endif

	using namespace griffin;
	using namespace griffin::resource;
	using namespace griffin::render;


	uint64_t griffin_tools_importMesh(const char* filename)
	{
		try {
			auto loader = g_resourceLoader.lock();
			if (!loader) {
				throw std::runtime_error("no resource loader");
			}

			// run the import task on the OpenGL thread since it creates GL resources
			task<uint64_t> tsk(ThreadAffinity::Thread_OpenGL_Render);

			tsk.run([loader, filename]{
				std::unique_ptr<Mesh_GL> meshPtr = importModelFile(filename);
				if (meshPtr == nullptr) {
					return 0ULL;
				}

				Mesh_GL* mesh = meshPtr.release();

				ResourcePtr meshResPtr = std::make_shared<Resource_T>(std::move(*mesh), mesh->getSize());

				// TODO: it makes no sense to load meshes into a "Materials" cache, pick another one
				auto res = loader->addResourceToCache<Mesh_GL>(meshResPtr, CacheType::Cache_Models_T);

				return res.resourceId.get().value;

			});

			return tsk.get();
		}
		catch (std::exception ex) {
			SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "griffin_tools_importMesh exception: %s", ex.what());
			return 0;
		}
	}

	
	bool griffin_tools_saveMesh(uint64_t mesh, const char* filename)
	{
		try {
			auto loader = g_resourceLoader.lock();
			if (!loader) {
				throw std::runtime_error("no resource loader");
			}

			Id_T handle;
			handle.value = mesh;

			// TODO: change cache to same as import function
			auto res = loader->getResource<Mesh_GL>(handle, CacheType::Cache_Models_T);

			std::ofstream ofs;
			SDL_Log("saving mesh %llu to %s", mesh, filename);
			ofs.open(filename, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
			ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);

			res->serialize(ofs);

			ofs.close();

			return true;
		}
		catch (std::exception ex) {
			SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "griffin_tools_saveMesh exception: %s", ex.what());
			return false;
		}
	}


	uint64_t griffin_tools_convertMesh(const char* sourceFilename, const char* destFilename)
	{
		uint64_t mesh = griffin_tools_importMesh(sourceFilename);
		if (mesh == 0) {
			return 0;
		}
		
		bool saved = griffin_tools_saveMesh(mesh, destFilename);
		if (!saved) {
			return 0;
		}
		
		return mesh;
	}

#ifdef __cplusplus
}
#endif