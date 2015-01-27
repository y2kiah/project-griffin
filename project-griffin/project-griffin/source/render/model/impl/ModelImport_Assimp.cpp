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
#include <queue>
#include <limits>

#include <render/model/Mesh_GL.h>
#include <render/material/Material.h>
#include <glm/vec4.hpp>

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
		void     fillMaterials(const aiScene&, Material*, size_t);
		uint32_t getMeshSceneNodeArraySize(const aiScene&);
		uint32_t getChildIndicesArraySize(const aiScene&);
		uint32_t getMeshIndicesArraySize(const aiScene&);
		void     fillSceneGraph(const aiScene&, MeshSceneGraph&);


		/**
		* Imports a model using assimp. Call this from the OpenGL thread only.
		* @returns "unique_ptr holding the loaded mesh, or nullptr on error"
		*/
		std::unique_ptr<Mesh_GL> importModelFile(const string &filename)
		{
			Importer importer;
			importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);

			uint32_t ppFlags = aiProcessPreset_TargetRealtime_MaxQuality |
				aiProcess_TransformUVCoords;

			const aiScene* p_scene = importer.ReadFile(filename, ppFlags);

			if (!p_scene) {
				//debugPrintf("Mesh::importFromFile: failed: %s\n", importer.GetErrorString());
				return nullptr;
			}
			auto& scene = *p_scene;

			// get materials
			uint32_t numMaterials = scene.mNumMaterials;
			auto materials = std::make_unique<Material[]>(numMaterials);
			fillMaterials(scene, materials.get(), numMaterials);

			// get meshes, vertex and index buffers
			uint32_t numMeshes = scene.mNumMeshes;
			auto drawSets = std::make_unique<DrawSet[]>(numMeshes);

			uint32_t totalVertexBufferSize = getTotalVertexBufferSize(scene, drawSets.get());
			auto vertexBuffer = std::make_unique<unsigned char[]>(totalVertexBufferSize);
			uint32_t numVertices = fillVertexBuffer(scene, vertexBuffer.get(), totalVertexBufferSize);
			
			uint32_t totalIndexBufferSize = getTotalIndexBufferSize(scene, drawSets.get(), numVertices);
			auto indexBuffer = std::make_unique<unsigned char[]>(totalIndexBufferSize);
			uint32_t numElements = fillIndexBuffer(scene, indexBuffer.get(), totalIndexBufferSize, numVertices, drawSets.get());

			VertexBuffer_GL vb;
			vb.loadFromMemory(vertexBuffer.get(), totalVertexBufferSize);

			IndexBuffer_GL ib;
			ib.loadFromMemory(indexBuffer.get(), totalIndexBufferSize);

			// get mesh scene graph
			uint32_t numMeshSceneNodes = getMeshSceneNodeArraySize(scene);
			uint32_t numChildIndices = getChildIndicesArraySize(scene);
			uint32_t numMeshIndices = getMeshIndicesArraySize(scene);
			size_t totalSceneGraphSize = (numMeshSceneNodes * sizeof(MeshSceneNode)) + 
										 ((numChildIndices + numMeshIndices) * sizeof(uint32_t));
			auto sceneGraphData = std::make_unique<unsigned char[]>(totalSceneGraphSize);

			// build the mesh object
			auto meshPtr = std::make_unique<Mesh_GL>(numMeshes, std::move(drawSets), std::move(vb), std::move(ib));
			
			// everything "assimp" is cleaned up by importer destructor
			return meshPtr;
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

					drawSet.vertexSize = thisVertexSize;
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


		/**
		* fill materials buffer
		*/
		void fillMaterials(const aiScene& scene, Material* materials,
						   size_t numMaterials)
		{
			for (uint32_t m = 0; m < numMaterials; ++m) {
				auto assimpMat = scene.mMaterials[m];
				auto& mat = materials[m];

				aiColor4D color;
				aiGetMaterialColor(assimpMat, AI_MATKEY_COLOR_DIFFUSE, &color);
				mat.diffuseColor = { color.r, color.g, color.b };

				aiGetMaterialColor(assimpMat, AI_MATKEY_COLOR_AMBIENT, &color);
				mat.ambientColor = { color.r, color.g, color.b };

				aiGetMaterialColor(assimpMat, AI_MATKEY_COLOR_SPECULAR, &color);
				mat.specularColor = { color.r, color.g, color.b };

				aiGetMaterialColor(assimpMat, AI_MATKEY_COLOR_EMISSIVE, &color);
				mat.emissiveColor = { color.r, color.g, color.b };

				aiGetMaterialColor(assimpMat, AI_MATKEY_COLOR_REFLECTIVE, &color);
				mat.reflectiveColor = { color.r, color.g, color.b };

				aiGetMaterialColor(assimpMat, AI_MATKEY_COLOR_TRANSPARENT, &color);
				mat.transparentColor = { color.r, color.g, color.b };


				// TEMP, just set to default effect. This should be read from one of the material properties if possible
				//mat.setEffect(L"data/shaders/Default.ifx", false);

				// for each texture type
				uint32_t samplerIndex = 0;
				for (uint32_t tt = 0; tt < AI_TEXTURE_TYPE_MAX; ++tt) {
					// for each texture of a type
					for (uint32_t i = 0; i < assimpMat->GetTextureCount((aiTextureType)tt); ++i) {
						aiString path;
						assimpMat->GetTexture((aiTextureType)tt, i, &path); // get the name, ignore other attributes for now

						// convert from ascii to wide character set, may need to also add a prefix for resource location
						string aPath(path.data);
						wstring wPath;
						wPath.assign(aPath.begin(), aPath.end());

						// do synchronous loading since this is a utility function
						// if needed could also build the texture here and inject into cache, then pass assumeCached=true
						//mat.addTexture(wPath, samplerIndex, false);

						++samplerIndex;
					}
				}
			}
		}


		template <typename F>
		void traverseSceneGraph(const aiScene& scene, F func)
		{
			struct BFSQueueItem {
				aiNode * node;
				uint32_t parentIndex;
			};

			std::queue<BFSQueueItem> bfsQueue; // queue used for breadth-first traversal

			uint32_t index = 0;

			bfsQueue.push({ scene.mRootNode, 0 });

			while (!bfsQueue.empty()) {
				auto& item = bfsQueue.front();
				bfsQueue.pop();
				// pass the node, the breadth-first index, and the parent index to the lambda
				func(*item.node, index, item.parentIndex);

				for (uint32_t c = 0; c < item.node->mNumChildren; ++c) {
					bfsQueue.push({ item.node->mChildren[c], index });
				}

				++index;
			}
		}


		uint32_t getMeshSceneNodeArraySize(const aiScene& scene)
		{
			uint32_t count = 0;
			traverseSceneGraph(scene, [&](aiNode& assimpNode, uint32_t index, uint32_t parentIndex) {
				++count;
			});
			return count;
		}


		uint32_t getChildIndicesArraySize(const aiScene& scene)
		{
			uint32_t totalChildren = 0;
			traverseSceneGraph(scene, [&totalChildren](aiNode& assimpNode, uint32_t index, uint32_t parentIndex) {
				totalChildren += assimpNode.mNumChildren;
			});
			return totalChildren;
		}


		uint32_t getMeshIndicesArraySize(const aiScene& scene)
		{
			uint32_t totalMeshes = 0;
			traverseSceneGraph(scene, [&totalMeshes](aiNode& assimpNode, uint32_t index, uint32_t parentIndex) {
				totalMeshes += assimpNode.mNumMeshes;
			});
			return totalMeshes;
		}


		void fillSceneGraph(const aiScene& scene, MeshSceneGraph& sceneGraph)
		{
			uint32_t childIndexOffset = 0;
			uint32_t meshIndexOffset = 0;

			traverseSceneGraph(scene, [&](aiNode& assimpNode, uint32_t index, uint32_t parentIndex) {
				auto& thisNode = sceneGraph.sceneNodes[index];

				// copy node data
				auto& t = assimpNode.mTransformation;
				thisNode.transform = { t.a1, t.a2, t.a3, t.a4, t.b1, t.b2, t.b3, t.b4, t.c1, t.c2, t.c3, t.c4, t.d1, t.d2, t.d3, t.d4 };
				thisNode.numChildren = assimpNode.mNumChildren;
				thisNode.numMeshes = assimpNode.mNumMeshes;
				thisNode.childIndexOffset = childIndexOffset;
				thisNode.meshIndexOffset = meshIndexOffset;
				thisNode.parentIndex = parentIndex;

				// still need to populate child and mesh index arrays

				// advance the counters for the next node
				childIndexOffset += assimpNode.mNumChildren;
				meshIndexOffset += assimpNode.mNumMeshes;
			});
			
		}
	}
}

#endif