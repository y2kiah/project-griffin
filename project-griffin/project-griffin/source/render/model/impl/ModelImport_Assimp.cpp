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
		std::tuple<uint32_t, uint32_t, uint32_t> getSceneArraySizes(const aiScene&);
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
				aiProcess_TransformUVCoords | aiProcess_OptimizeGraph | aiProcess_PreTransformVertices;

			const aiScene* p_scene = importer.ReadFile(filename, ppFlags);

			if (!p_scene) {
				//debugPrintf("Mesh::importFromFile: failed: %s\n", importer.GetErrorString());
				return nullptr;
			}
			auto& scene = *p_scene;

			// get drawsets size
			uint32_t numMeshes = scene.mNumMeshes;
			size_t totalDrawSetsSize = numMeshes * sizeof(DrawSet);

			// get materials size
			uint32_t numMaterials = scene.mNumMaterials;
			size_t totalMaterialsSize = numMaterials * sizeof(Material);

			// get mesh scene graph size
			MeshSceneGraph meshScene;
			auto sceneTuple = getSceneArraySizes(scene);
			meshScene.numNodes = std::get<0>(sceneTuple);
			meshScene.numChildIndices = std::get<1>(sceneTuple);
			meshScene.numMeshIndices = std::get<2>(sceneTuple);

			size_t totalSceneGraphSize = (meshScene.numNodes * sizeof(MeshSceneNode)) +
										 ((meshScene.numChildIndices + meshScene.numMeshIndices) * sizeof(uint32_t));

			// create model data buffer
			size_t modelDataSize = totalDrawSetsSize + totalMaterialsSize + totalSceneGraphSize;
			auto modelData = std::make_unique<unsigned char[]>(modelDataSize);

			// get meshes, vertex and index buffers
			DrawSet* drawSets = reinterpret_cast<DrawSet*>(modelData.get());

			uint32_t totalVertexBufferSize = getTotalVertexBufferSize(scene, drawSets);
			auto vertexBuffer = std::make_unique<unsigned char[]>(totalVertexBufferSize);
			uint32_t numVertices = fillVertexBuffer(scene, vertexBuffer.get(), totalVertexBufferSize);
			
			uint32_t totalIndexBufferSize = getTotalIndexBufferSize(scene, drawSets, numVertices);
			auto indexBuffer = std::make_unique<unsigned char[]>(totalIndexBufferSize);
			uint32_t numElements = fillIndexBuffer(scene, indexBuffer.get(), totalIndexBufferSize, numVertices, drawSets);
			int sizeOfElement = totalIndexBufferSize / numElements;

			VertexBuffer_GL vb;
			vb.loadFromMemory(vertexBuffer.get(), totalVertexBufferSize);

			IndexBuffer_GL ib;
			ib.loadFromMemory(indexBuffer.get(), totalIndexBufferSize, sizeOfElement);

			// fill materials
			uint32_t materialsOffset = static_cast<uint32_t>(totalDrawSetsSize);
			Material* materials = reinterpret_cast<Material*>(modelData.get() + materialsOffset);
			fillMaterials(scene, materials, numMaterials);

			// fill scene graph
			uint32_t meshSceneOffset = static_cast<uint32_t>(totalDrawSetsSize + totalMaterialsSize);
			meshScene.childIndicesOffset = (meshScene.numNodes * sizeof(MeshSceneNode));
			meshScene.meshIndicesOffset  = meshScene.childIndicesOffset + (meshScene.numChildIndices * sizeof(uint32_t));
			meshScene.sceneNodes   = reinterpret_cast<MeshSceneNode*>(modelData.get() + meshSceneOffset);
			meshScene.childIndices = reinterpret_cast<uint32_t*>(modelData.get() + meshSceneOffset + meshScene.childIndicesOffset);
			meshScene.meshIndices  = reinterpret_cast<uint32_t*>(modelData.get() + meshSceneOffset + meshScene.meshIndicesOffset);
			fillSceneGraph(scene, meshScene);

			// build the mesh object
			auto meshPtr = std::make_unique<Mesh_GL>(modelDataSize, numMeshes, numMaterials,
													 materialsOffset, meshSceneOffset, std::move(modelData),
													 std::move(meshScene), std::move(vb), std::move(ib));
			
			// everything "assimp" is cleaned up by importer destructor
			return meshPtr;
		}


		// Vertex Buffer Loading

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


		// Index Buffer Loading

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
						const auto& face = assimpMesh.mFaces[f];

						switch (sizeofElement) {
							case sizeof(uint32_t) : {
								uint32_t indices[3] = {
									face.mIndices[0],
									face.mIndices[1],
									face.mIndices[2]
								};
								memcpy_s(p_ib, bufferSize - (p_ib - buffer),
										 indices, sizeofElement * 3);
								break;
							}
							case sizeof(uint16_t) : {
								uint16_t indices[3] = {
									static_cast<uint16_t>(face.mIndices[0]),
									static_cast<uint16_t>(face.mIndices[1]),
									static_cast<uint16_t>(face.mIndices[2])
								};
								memcpy_s(p_ib, bufferSize - (p_ib - buffer),
										 indices, sizeofElement * 3);
								break;
							}
							case sizeof(uint8_t) : {
								uint8_t indices[3] = {
									static_cast<uint8_t>(face.mIndices[0]),
									static_cast<uint8_t>(face.mIndices[1]),
									static_cast<uint8_t>(face.mIndices[2])
								};
								memcpy_s(p_ib, bufferSize - (p_ib - buffer),
										 indices, sizeofElement * 3);
								break;
							}
						}

						lowest  = std::min(lowest,  face.mIndices[0]);
						lowest  = std::min(lowest,  face.mIndices[1]);
						lowest  = std::min(lowest,  face.mIndices[2]);
						highest = std::max(highest, face.mIndices[0]);
						highest = std::max(highest, face.mIndices[1]);
						highest = std::max(highest, face.mIndices[2]);

						p_ib += sizeofElement * 3;
					}

					drawSet.indexRangeStart = lowest;
					drawSet.indexRangeEnd = highest;
				}
			}

			return totalElements;
		}


		// Material Loading

		/**
		* fill materials buffer
		*/
		void fillMaterials(const aiScene& scene, Material* materials,
						   size_t numMaterials)
		{
			for (uint32_t m = 0; m < numMaterials; ++m) {
				auto assimpMat = scene.mMaterials[m];
				auto& mat = materials[m];
				
				aiString name;
				aiGetMaterialString(assimpMat, AI_MATKEY_NAME, &name);
				// store name in material?

				aiGetMaterialFloat(assimpMat, AI_MATKEY_OPACITY, &mat.opacity);
				aiGetMaterialFloat(assimpMat, AI_MATKEY_REFLECTIVITY, &mat.reflectivity);
				aiGetMaterialFloat(assimpMat, AI_MATKEY_REFRACTI, &mat.refracti);
				aiGetMaterialFloat(assimpMat, AI_MATKEY_SHININESS, &mat.shininess);
				aiGetMaterialFloat(assimpMat, AI_MATKEY_SHININESS_STRENGTH, &mat.shininessStrength);

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


		// Scene Graph Loading

		template <typename F>
		void traverseSceneGraph(const aiScene& scene, F func)
		{
			struct BFSQueueItem {
				aiNode* node;
				int     parentIndex;
				int     childIndexOfParent;
			};

			std::queue<BFSQueueItem> bfsQueue; // queue used for breadth-first traversal

			uint32_t index = 0;

			// push the root node into the scene graph to start traversal
			bfsQueue.push({ scene.mRootNode, -1, 0 });

			while (!bfsQueue.empty()) {
				auto& item = bfsQueue.front();
				
				// pass the node, the breadth-first index, and the parent index to the lambda
				func(*item.node, index, item.parentIndex, item.childIndexOfParent);

				for (uint32_t c = 0; c < item.node->mNumChildren; ++c) {
					aiNode* childNode = item.node->mChildren[c];
					bfsQueue.push({ childNode, index, c });
				}

				bfsQueue.pop();
				++index;
			}
		}


		/**
		* @returns tuple of node count, child count and mesh count
		*/
		std::tuple<uint32_t, uint32_t, uint32_t> getSceneArraySizes(const aiScene& scene)
		{
			uint32_t count = 0;
			uint32_t totalChildren = 0;
			uint32_t totalMeshes = 0;
			
			traverseSceneGraph(scene, [&](aiNode& assimpNode, uint32_t index,
										  int parentIndex, int childIndexOfParent)
			{
				++count;
				totalChildren += assimpNode.mNumChildren;
				totalMeshes += assimpNode.mNumMeshes;
			});

			return std::make_tuple(count, totalChildren, totalMeshes);
		}


		void fillSceneGraph(const aiScene& scene, MeshSceneGraph& sceneGraph)
		{
			uint32_t childIndexOffset = 0;
			uint32_t meshIndexOffset = 0;

			traverseSceneGraph(scene, [&](aiNode& assimpNode, uint32_t index,
										  int parentIndex, int childIndexOfParent)
			{
				// parent index of -1 means this is the root node
				if (parentIndex != -1) {
					// populate this index into child indices array, relative to parent's child offset
					auto& parentNode = sceneGraph.sceneNodes[parentIndex];
					sceneGraph.childIndices[parentNode.childIndexOffset + childIndexOfParent] = index;
				}

				// copy node data
				auto& thisNode = sceneGraph.sceneNodes[index];

				auto& t = assimpNode.mTransformation;
				thisNode.transform = { t.a1, t.b1, t.c1, t.d1,
									   t.a2, t.b2, t.c2, t.d2,
									   t.a3, t.b3, t.c3, t.d3,
									   t.a4, t.b4, t.c4, t.d4 };
				thisNode.numChildren = assimpNode.mNumChildren;
				thisNode.numMeshes = assimpNode.mNumMeshes;
				thisNode.childIndexOffset = childIndexOffset;
				thisNode.meshIndexOffset = meshIndexOffset;
				thisNode.parentIndex = parentIndex;
				thisNode.name = assimpNode.mName.C_Str();

				// populate mesh index array
				for (unsigned int m = 0; m < thisNode.numMeshes; ++m) {
					sceneGraph.meshIndices[meshIndexOffset + m] = assimpNode.mMeshes[m];
				}

				// advance the counters for the next node
				childIndexOffset += assimpNode.mNumChildren;
				meshIndexOffset += assimpNode.mNumMeshes;
			});
		}

	}
}

#endif