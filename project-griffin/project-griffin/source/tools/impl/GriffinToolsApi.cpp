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
#include <fstream>
#include <utility/Logger.h>

namespace griffin {
	namespace tools {
		griffin::resource::ResourceLoaderWeakPtr g_resourceLoader;

		void setResourceLoaderPtr(const resource::ResourceLoaderPtr& resourcePtr)
		{
			g_resourceLoader = resourcePtr;
		}
	}
}

#ifdef __cplusplus
extern "C" {
#endif

	using namespace griffin;
	using namespace griffin::resource;
	using namespace griffin::render;


	uint64_t griffin_tools_importMesh(const char* filename, bool optimizeGraph, bool preTransformVertices, bool flipUVs)
	{
		try {
			auto loader = tools::g_resourceLoader.lock();
			if (!loader) {
				throw std::runtime_error("no resource loader");
			}

			// run the import task on the OpenGL thread since it creates GL resources
			task<uint64_t> tsk(ThreadAffinity::Thread_OpenGL_Render);

			tsk.run([loader, filename, optimizeGraph, preTransformVertices, flipUVs]{
				std::unique_ptr<Mesh_GL> meshPtr = importModelFile(filename, optimizeGraph, preTransformVertices, flipUVs);
				if (meshPtr == nullptr) {
					return 0ULL;
				}

				Mesh_GL* mesh = meshPtr.release();

				ResourcePtr meshResPtr = std::make_shared<Resource_T>(std::move(*mesh), mesh->getSize());

				auto res = loader->addResourceToCache<Mesh_GL>(meshResPtr, CacheType::Cache_Models);
				res.resourceId.wait();

				return res.resourceId.get().value;

			});

			return tsk.get();
		}
		catch (std::exception ex) {
			logger.warn(Logger::Category_Error, "griffin_tools_importMesh exception: %s", ex.what());
			return 0;
		}
	}

	
	bool griffin_tools_saveMesh(uint64_t mesh, const char* filename)
	{
		try {
			auto loader = tools::g_resourceLoader.lock();
			if (!loader) {
				throw std::runtime_error("no resource loader");
			}

			Id_T handle;
			handle.value = mesh;

			auto resPtr = loader->getResource(handle, CacheType::Cache_Models);
			auto& res = resPtr->getResource<Mesh_GL>();

			std::ofstream ofs;
			logger.info("saving mesh %llu to %s", mesh, filename);
			ofs.open(filename, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
			ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);

			res.serialize(ofs);

			ofs.close();

			return true;
		}
		catch (std::exception ex) {
			logger.warn(Logger::Category_Error, "griffin_tools_saveMesh exception: %s", ex.what());
			return false;
		}
	}


	uint64_t griffin_tools_convertMesh(const char* sourceFilename, const char* destFilename,
									   bool optimizeGraph, bool preTransformVertices, bool flipUVs)
	{
		uint64_t mesh = griffin_tools_importMesh(sourceFilename, optimizeGraph, preTransformVertices, flipUVs);
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