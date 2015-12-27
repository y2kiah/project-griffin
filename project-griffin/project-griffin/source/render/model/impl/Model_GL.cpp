/**
* @file Model_GL.cpp
* @author Jeff Kiah
*/
#include <render/model/Model_GL.h>
#include <application/Engine.h>
//#include <GL/glew.h>
//#include <utility>
//#include <cassert>
//#include <render/ShaderProgramLayouts_GL.h>
#include <render/Render.h>
#include <render/RenderResources.h>
#include <resource/ResourceLoader.h>
//#include <utility/container/vector_queue.h>
//#include <glm/mat4x4.hpp>
//#include <glm/gtc/matrix_transform.hpp>
#include <scene/Scene.h>
#include <glm/mat4x4.hpp>
//#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_access.hpp>


namespace griffin {
	namespace render {
		using namespace glm;

		void Model_GL::render(Id_T entityId, scene::Scene& scene, uint8_t viewport, Engine& engine)
		{
			//m_mesh.render(engine, viewport );
		}


		void Model_GL::draw(Id_T entityId, int drawSetIndex)
		{
			m_mesh.drawMesh(drawSetIndex);
		}


		void Model_GL::initRenderEntries()
		{
			m_renderEntries.keys.reserve(m_mesh.m_meshScene.numMeshIndices);
			m_renderEntries.entries.reserve(m_mesh.m_meshScene.numMeshIndices);

			struct BFSQueueItem {
				uint32_t nodeIndex;
				dmat4    modelTransform;
			};

			vector_queue<BFSQueueItem> bfsQueue;
			bfsQueue.reserve(m_mesh.m_meshScene.numNodes);

			bfsQueue.push({ 0, dmat4() }); // push root node to start traversal

			while (!bfsQueue.empty()) {
				auto& thisItem = bfsQueue.front();

				uint32_t nodeIndex = thisItem.nodeIndex;
				assert(nodeIndex >= 0 && nodeIndex < m_mesh.m_meshScene.numNodes && "node index out of range");

				const auto& node = m_mesh.m_meshScene.sceneNodes[thisItem.nodeIndex];

				dmat4 localTransform = thisItem.modelTransform * dmat4(node.transform);
				dvec4 translation = localTransform[3];
				dquat orientation = normalize(quat_cast(localTransform));
				dvec3 scaling{ length(column(localTransform, 0)), length(column(localTransform, 1)), length(column(localTransform, 2)) };
				// TODO: keep an eye on calcs above, assumes there is no skew, use function below if bugs appear
				//decompose(localTransform, scaling, orientation, translation, dvec3(), dvec4());

				// draw this node's meshes
				for (uint32_t m = 0; m < node.numMeshes; ++m) {
					using namespace std::placeholders;

					uint32_t ds = m_mesh.m_meshScene.meshIndices[node.meshIndexOffset + m];
					auto& drawSet = m_mesh.m_drawSets[ds];
					Material_GL& mat = m_mesh.m_materials[drawSet.materialIndex];

					RenderQueueKey key{};
					key.allKeys.sceneLayer = SceneLayer_SceneGeometry;

					if (mat.shaderKey.isTranslucent == 1 && mat.shaderKey.usesAlphaBlend == 1) {
						key.allKeys.sceneLayer = SceneLayer_Translucent;
						key.translucentKey.translucencyType = TranslucencyType_AlphaBlend;
					}
					else if (mat.shaderKey.isTranslucent == 1 && mat.shaderKey.usesAlphaTest == 1) {
						key.allKeys.sceneLayer = SceneLayer_Translucent;
						key.translucentKey.translucencyType = TranslucencyType_AlphaTest;
					}
					key.allKeys.fullscreenLayer = FullscreenLayer_Scene;

					// add key and entry to returned list
					m_renderEntries.keys.push_back({
						key,
						static_cast<uint32_t>(m_renderEntries.entries.size())
					});

					m_renderEntries.entries.emplace_back(RenderEntry{
						translation,
						orientation,
						scaling,
						NullId_T,
						std::bind(&Model_GL::draw, this, _1, _2),
						ds
					});
				}

				// push children to traverse
				for (uint32_t c = 0; c < node.numChildren; ++c) {
					uint32_t childNodeIndex = m_mesh.m_meshScene.childIndices[node.childIndexOffset + c];

					assert(childNodeIndex >= 0 && childNodeIndex < m_mesh.m_meshScene.numNodes && "child node index out of range");
					assert(childNodeIndex > nodeIndex && "child node is not lower in the tree");

					bfsQueue.push({ childNodeIndex, localTransform });
				}

				bfsQueue.pop();
			}

			assert(m_renderEntries.keys.capacity() == m_mesh.m_meshScene.numMeshIndices && "total draw calls should be equal to total mesh indices");
		}


		// TODO: finish this function, store handles to resources, load shader resource
		//	(but don't cause opengl calls directly... may have to do shader compile in yet another function)
		void Model_GL::loadMaterialResources(const std::wstring& filePath)
		{
			// load shaders and textures
			for (uint32_t m = 0; m < m_mesh.m_numMaterials; ++m) {
				auto& mat = m_mesh.m_materials[m];
				for (uint32_t t = 0; t < mat.numTextures; ++t) {
					auto& tex = mat.textures[t];
					if (tex.textureType != MaterialTexture_None) {
						// convert from ascii to wide character set
						string aName(tex.name);
						wstring wName;
						wName.assign(aName.begin(), aName.end());

						// prefix texture path with the path to the model being loaded
						wName = filePath.substr(0, filePath.find_last_of(L'/')) + L'/' + wName;
						SDL_Log("trying to load %s", aName.assign(wName.begin(), wName.end()).c_str());

						// TEMP, right now loading is blocking, called on opengl thread. Once tasks are used, switch to calling from the loading thread.
						auto resHandle = render::loadTexture2D(wName, resource::CacheType::Cache_Materials);

						// TEMP, blocking call, need to make this async, use task system
						// BUT, the continuation must update this handle, assuming "this" pointer is captured by reference,
						// the material may move in memory, since the resource system is free to move it.
						// Potential bug, use the resource ids to look up by handle to get its current memory location from the task
						// Also, can this model move in memory while tasks are queued holding a memory location back? Don't allow that to happen.
						tex.textureResourceHandle = resHandle.handle();
						auto texPtr = render::g_resourceLoader.lock()->getResource(tex.textureResourceHandle, resource::CacheType::Cache_Materials);
						auto& tex = texPtr->getResource<Texture2D_GL>();
						
						// TODO: where should I call the following??? Can't call here since this is called on loading thread
						//pTex->bind(GL_TEXTURE0);
						//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					}
				}
			}
		}

	}
}