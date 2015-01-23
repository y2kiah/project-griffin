#ifdef GRIFFIN_TOOLS_BUILD

#pragma comment( lib, "assimp.lib" )

#include "../ModelImport_Assimp.h"
#include <gl/glew.h>

#include <assimp/Importer.hpp>	// C++ importer interface
#include <assimp/scene.h>		// Output data structure
#include <assimp/postprocess.h>	// Post processing flags
#include <assimp/config.h>		// Configuration properties

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <limits>

#include "../Mesh_GL.h"

using namespace Assimp;
using std::string;
using std::wstring;
using std::unique_ptr;
using std::vector;

namespace griffin {
	namespace render {

		// Forward Declarations

		uint32_t getTotalVertexBufferSize(const aiScene&, DrawSet*);
		uint32_t fillVertexBuffer(const aiScene&, unsigned char*, size_t);
		uint32_t getTotalIndexBufferSize(const aiScene&, DrawSet*, size_t);
		uint32_t fillIndexBuffer(const aiScene&, unsigned char*, size_t, size_t, DrawSet*);


		/**
		* Imports a model using assimp
		*/
		bool importModelFile(const string &filename, Mesh_GL& loadMesh)
		{
			Importer importer;
			importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);

			uint32_t ppFlags = aiProcessPreset_TargetRealtime_MaxQuality |
				aiProcess_TransformUVCoords;

			const aiScene* scene = importer.ReadFile(filename, ppFlags);

			if (!scene) {
				//debugPrintf("Mesh::importFromFile: failed: %s\n", importer.GetErrorString());
				return false;
			}

			// get Materials
			/*	m_materials.reserve(scene->mNumMaterials);
				for (uint32_t m = 0; m < scene->mNumMaterials; ++m) {
				MaterialPtr matPtr(new Material());
				m_materials.push_back(matPtr);

				// TEMP, just set to default effect. This should be read from one of the material properties if possible
				matPtr->setEffect(L"data/shaders/Default.ifx", false);

				// for each texture type
				uint32_t samplerIndex = 0;
				for (uint32_t tt = 0; tt < AI_TEXTURE_TYPE_MAX; ++tt) {
				// for each texture of a type
				for (uint32_t i = 0; i < scene->mMaterials[m]->GetTextureCount((aiTextureType)tt); ++i) {
				aiString path;
				scene->mMaterials[m]->GetTexture((aiTextureType)tt, i, &path); // get the name, ignore other attributes for now

				// convert from ascii to wide character set, may need to also add a prefix for resource location
				string aPath(path.data);
				wstring wPath;
				wPath.assign(aPath.begin(), aPath.end());

				// do synchronous loading since this is a utility function
				// if needed could also build the texture here and inject into cache, then pass assumeCached=true
				matPtr->addTexture(wPath, samplerIndex, false);

				++samplerIndex;
				}
				}
				}
				*/
			

			uint32_t numMeshes = scene->mNumMeshes;
			auto drawSets = std::make_unique<DrawSet[]>(numMeshes);

			uint32_t totalVertexBufferSize = getTotalVertexBufferSize(*scene, drawSets.get());
			auto vertexBuffer = std::make_unique<unsigned char[]>(totalVertexBufferSize);
			uint32_t numVertices = fillVertexBuffer(*scene, vertexBuffer.get(), totalVertexBufferSize);
			
			uint32_t totalIndexBufferSize = getTotalIndexBufferSize(*scene, drawSets.get(), numVertices);
			auto indexBuffer = std::make_unique<unsigned char[]>(totalIndexBufferSize);
			uint32_t numElements = fillIndexBuffer(*scene, indexBuffer.get(), totalIndexBufferSize, numVertices, drawSets.get());

			VertexBuffer_GL vb;
			vb.loadFromMemory(vertexBuffer.get(), totalVertexBufferSize);

			IndexBuffer_GL ib;
			ib.loadFromMemory(indexBuffer.get(), totalIndexBufferSize);

			auto meshPtr = std::make_unique<Mesh_GL>(numMeshes, std::move(drawSets), std::move(vb), std::move(ib));

			// get Scene graph
			/*	std::function<void(aiNode*, MeshNode&)> traverseScene;
				traverseScene = [&traverseScene](aiNode *node, MeshNode &thisNode) {
				// set mesh indices
				for (uint32_t m = 0; m < node->mNumMeshes; ++m) {
				thisNode.m_meshIndex.push_back(node->mMeshes[m]);
				}
				// set child nodes
				for (uint32_t c = 0; c < node->mNumChildren; ++c) {
				// convert the transform matrix from aiMatrix4x4 to Matrix4x4f
				aiMatrix4x4 &t = node->mChildren[c]->mTransformation;
				const float pTransform[16] = { t.a1, t.a2, t.a3, t.a4, t.b1, t.b2, t.b3, t.b4, t.c1, t.c2, t.c3, t.c4, t.d1, t.d2, t.d3, t.d4 };
				Matrix4x4f transform(pTransform);
				// create and add the new node
				SceneNodePtr newNode(new MeshNode(transform, node->mChildren[c]->mName.data));
				thisNode.addChild(newNode);
				// recursive call into child node
				traverseScene(node->mChildren[c], *(static_cast<MeshNode*>(newNode.get())));
				}
				};
				// start building scene at root
				traverseScene(scene->mRootNode, *(static_cast<MeshNode*>(m_root.get())));

				// set the model name
				mName.assign(filename.begin(), filename.end());
				*/

			// everything "assimp" is cleaned up by importer destructor

			return true;
		}


		/**
		* @drawSets pointer to array of DrawSet, filled with vertex buffer information
		* @returns total size of the model's single vertex buffer, including all meshes
		*/
		uint32_t getTotalVertexBufferSize(const aiScene& scene, DrawSet* drawSets)
		{
			uint32_t accumulatedVertexBufferSize = 0;

			uint32_t numMeshes = scene.mNumMeshes;
			for (uint32_t m = 0; m < numMeshes; ++m) {
				auto& assimpMesh = *scene.mMeshes[m];
				auto& drawSet = drawSets[m];

				drawSet.vertexBaseOffset = accumulatedVertexBufferSize;
				drawSet.numTexCoordChannels = assimpMesh.GetNumUVChannels();
				drawSet.numColorChannels = assimpMesh.GetNumColorChannels();

				if (assimpMesh.mPrimitiveTypes == aiPrimitiveType_TRIANGLE) {
					drawSet.glPrimitiveType = GL_TRIANGLES;

					uint8_t thisVertexSize = 0;
					if (assimpMesh.HasPositions()) {
						drawSet.vertexFlags |= Vertex_Positions;
						thisVertexSize += sizeof(float) * 3;
					}
					if (assimpMesh.HasNormals()) {
						drawSet.vertexFlags |= Vertex_Normals;
						drawSet.normalOffset = thisVertexSize;

						thisVertexSize += sizeof(float) * 3;
					}
					if (assimpMesh.HasTangentsAndBitangents()) {
						drawSet.vertexFlags |= Vertex_TangentsAndBitangents;
						drawSet.tangentOffset = thisVertexSize;
						drawSet.bitangentOffset = thisVertexSize + sizeof(float) * 3;

						thisVertexSize += sizeof(float) * 3 * 2;
					}

					if (drawSet.numTexCoordChannels > 0) {
						drawSet.vertexFlags |= Vertex_TextureCoords;
						drawSet.texCoordsOffset = thisVertexSize;

						for (uint32_t c = 0; c < drawSet.numTexCoordChannels; ++c) {
							if (assimpMesh.HasTextureCoords(c)) {
								uint8_t numTexCoordComponents = assimpMesh.mNumUVComponents[c];
								drawSet.numTexCoordComponents[c] = numTexCoordComponents;

								thisVertexSize += sizeof(float) * numTexCoordComponents;
							}
						}
					
					}
					
					if (drawSet.numColorChannels > 0) {
						drawSet.vertexFlags |= Vertex_Colors;

						for (uint32_t c = 0; c < drawSet.numColorChannels; ++c) {
							if (assimpMesh.HasVertexColors(c)) {
								thisVertexSize += sizeof(float) * 4;
							}
						}
					}

					uint32_t thisVertexBufferSize = assimpMesh.mNumVertices * thisVertexSize;
					accumulatedVertexBufferSize += thisVertexBufferSize;
				}
			}

			return accumulatedVertexBufferSize;
		}


		/**
		* fill vertex buffer
		* @returns the number of vertices stored
		*/
		uint32_t fillVertexBuffer(const aiScene& scene, unsigned char* buffer, size_t bufferSize)
		{
			uint32_t totalVertices = 0;
			uint32_t numMeshes = scene.mNumMeshes;
			unsigned char* p_vb = buffer;
			const int sizeofVertex = sizeof(float) * 3;

			for (uint32_t m = 0; m < numMeshes; ++m) {
				auto& assimpMesh = *scene.mMeshes[m];

				// only looking for triangle meshes
				if (assimpMesh.mPrimitiveTypes == aiPrimitiveType_TRIANGLE) {
					uint32_t numVertices = assimpMesh.mNumVertices;
					totalVertices += numVertices;

					// for each vertex
					for (uint32_t v = 0; v < numVertices; ++v) {
						// copy vertex data into the buffer, use a running pointer to keep track
						// of the write position since these vertices are not always homogenous
						if (assimpMesh.HasPositions()) {
							memcpy_s(p_vb, bufferSize - (p_vb - buffer),
									 &assimpMesh.mVertices[v], sizeofVertex);
							p_vb += sizeofVertex;
						}
						if (assimpMesh.HasNormals()) {
							memcpy_s(p_vb, bufferSize - (p_vb - buffer),
									 &assimpMesh.mNormals[v], sizeofVertex);
							p_vb += sizeofVertex;
						}
						if (assimpMesh.HasTangentsAndBitangents()) {
							memcpy_s(p_vb, bufferSize - (p_vb - buffer),
									 &assimpMesh.mTangents[v], sizeofVertex);
							p_vb += sizeofVertex;
							memcpy_s(p_vb, bufferSize - (p_vb - buffer),
									 &assimpMesh.mBitangents[v], sizeofVertex);
							p_vb += sizeofVertex;
						}
						for (uint32_t c = 0; c < assimpMesh.GetNumUVChannels(); ++c) {
							if (assimpMesh.HasTextureCoords(c)) {
								auto uvSize = sizeof(float) * assimpMesh.mNumUVComponents[c];
								memcpy_s(p_vb, bufferSize - (p_vb - buffer),
										 &assimpMesh.mTextureCoords[c], uvSize);
								p_vb += uvSize;
							}
						}
						for (uint32_t c = 0; c < assimpMesh.GetNumColorChannels(); ++c) {
							if (assimpMesh.HasVertexColors(c)) {
								auto colorSize = sizeof(float) * 4;
								memcpy_s(p_vb, bufferSize - (p_vb - buffer),
										 &assimpMesh.mColors[c], colorSize);
								p_vb += colorSize;
							}
						}
					}
				}
			}

			return totalVertices;
		}


		/**
		* @drawSets pointer to array of DrawSet, filled with index buffer information
		* @returns total size of the model's index buffer, including all meshes
		*/
		uint32_t getTotalIndexBufferSize(const aiScene& scene, DrawSet* drawSets, size_t totalNumVertices)
		{
			// get total size of buffers
			uint32_t accumulatedIndexBufferSize = 0;

			// look at the total number of vertices in the model to find the highest
			// possible index value, use the smallest element that will fit it
			uint32_t sizeofElement = sizeof(uint32_t); // use a 32 bit index
			if (totalNumVertices <= std::numeric_limits<uint8_t>::max()) {
				sizeofElement = sizeof(uint8_t); // use 8 bit index
			}
			else if (totalNumVertices <= std::numeric_limits<uint16_t>::max()) {
				sizeofElement = sizeof(uint16_t); // use 16 bit index
			}

			uint32_t numMeshes = scene.mNumMeshes;
			for (uint32_t m = 0; m < numMeshes; ++m) {
				auto& assimpMesh = *scene.mMeshes[m];
				auto& drawSet = drawSets[m];

				drawSet.indexBaseOffset = accumulatedIndexBufferSize;

				if (assimpMesh.mPrimitiveTypes == aiPrimitiveType_TRIANGLE &&
					assimpMesh.HasFaces())
				{
					drawSet.numElements = assimpMesh.mNumFaces * 3;
					accumulatedIndexBufferSize += sizeofElement * drawSet.numElements;
				}
			}

			return accumulatedIndexBufferSize;
		}


		/**
		* fill index buffer
		* @returns the number of elements stored
		*/
		uint32_t fillIndexBuffer(const aiScene& scene, unsigned char* buffer,
								 size_t bufferSize, size_t totalNumVertices,
								 DrawSet* drawSets)
		{
			uint32_t totalElements = 0;
			uint32_t numMeshes = scene.mNumMeshes;
			unsigned char* p_ib = buffer;

			// look at the total number of vertices in the model to find the highest
			// possible index value, use the smallest element that will fit it
			auto sizeofElement = sizeof(uint32_t); // use a 32 bit index
			if (totalNumVertices <= std::numeric_limits<uint8_t>::max()) {
				sizeofElement = sizeof(uint8_t); // use 8 bit index
			}
			else if (totalNumVertices <= std::numeric_limits<uint16_t>::max()) {
				sizeofElement = sizeof(uint16_t); // use 16 bit index
			}

			for (uint32_t m = 0; m < numMeshes; ++m) {
				auto& assimpMesh = *scene.mMeshes[m];
				auto& drawSet = drawSets[m];

				// only looking for triangle meshes
				if (assimpMesh.mPrimitiveTypes == aiPrimitiveType_TRIANGLE &&
					assimpMesh.HasFaces())
				{
					uint32_t numFaces = assimpMesh.mNumFaces;
					uint32_t numElements = numFaces * 3;
					totalElements += numElements;

					uint32_t lowest = std::numeric_limits<uint32_t>::max();
					uint32_t highest = 0;

					// copy elements
					for (uint32_t f = 0; f < numFaces; ++f) {
						assert(assimpMesh.mFaces[f].mNumIndices == 3 && "the face doesn't have 3 indices");

						switch (sizeofElement) {
							case sizeof(uint32_t) : {
								uint32_t indices[3] = {
									assimpMesh.mFaces[f].mIndices[0],
									assimpMesh.mFaces[f].mIndices[1],
									assimpMesh.mFaces[f].mIndices[2]
								};
								memcpy_s(p_ib, bufferSize - (p_ib - buffer),
										 indices, sizeofElement * 3);
								break;
							}
							case sizeof(uint16_t) : {
								uint16_t indices[3] = {
									static_cast<uint16_t>(assimpMesh.mFaces[f].mIndices[0]),
									static_cast<uint16_t>(assimpMesh.mFaces[f].mIndices[1]),
									static_cast<uint16_t>(assimpMesh.mFaces[f].mIndices[2])
								};
								memcpy_s(p_ib, bufferSize - (p_ib - buffer),
										 indices, sizeofElement * 3);
								break;
							}
							case sizeof(uint8_t) : {
								uint8_t indices[3] = {
									static_cast<uint8_t>(assimpMesh.mFaces[f].mIndices[0]),
									static_cast<uint8_t>(assimpMesh.mFaces[f].mIndices[1]),
									static_cast<uint8_t>(assimpMesh.mFaces[f].mIndices[2])
								};
								memcpy_s(p_ib, bufferSize - (p_ib - buffer),
										 indices, sizeofElement * 3);
								break;
							}
						}

						lowest  = std::min(lowest,  assimpMesh.mFaces[f].mIndices[0]);
						lowest  = std::min(lowest,  assimpMesh.mFaces[f].mIndices[1]);
						lowest  = std::min(lowest,  assimpMesh.mFaces[f].mIndices[2]);
						highest = std::max(highest, assimpMesh.mFaces[f].mIndices[0]);
						highest = std::max(highest, assimpMesh.mFaces[f].mIndices[1]);
						highest = std::max(highest, assimpMesh.mFaces[f].mIndices[2]);

						p_ib += sizeofElement * 3;
					}

					drawSet.indexRangeStart = lowest;
					drawSet.indexRangeEnd = highest;
				}
			}

			return totalElements;
		}

	}
}

#endif