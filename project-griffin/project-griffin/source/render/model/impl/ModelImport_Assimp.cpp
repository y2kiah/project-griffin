/**
* @file ModelImport_Assimp.cpp
* @author Jeff Kiah
*/
#ifdef GRIFFIN_TOOLS_BUILD

#pragma comment( lib, "assimp.lib" )

#include <render/model/ModelImport_Assimp.h>
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
#include <render/material/Material_GL.h>
#include <glm/vec4.hpp>

#include <SDL_log.h>

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
		void     fillMaterials(const aiScene&, Material_GL*, size_t);
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
			size_t totalMaterialsSize = numMaterials * sizeof(Material_GL);

			// get mesh scene graph size
			MeshSceneGraph meshScene;
			auto sceneTuple = getSceneArraySizes(scene);
			meshScene.numNodes = std::get<0>(sceneTuple);
			meshScene.numChildIndices = std::get<1>(sceneTuple);
			meshScene.numMeshIndices = std::get<2>(sceneTuple);

			size_t totalSceneGraphSize = (meshScene.numNodes * sizeof(MeshSceneNode)) +
										 ((meshScene.numChildIndices + meshScene.numMeshIndices) * sizeof(uint32_t)) +
										 (meshScene.numNodes * sizeof(MeshSceneNodeMetaData));

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

			VertexBuffer_GL vb(std::move(vertexBuffer), totalVertexBufferSize);
			vb.loadFromInternalMemory(false); // don't discard the internal buffer so we can later serialize the mesh

			IndexBuffer_GL ib(std::move(indexBuffer), totalIndexBufferSize, IndexBuffer_GL::getSizeFlag(sizeOfElement));
			ib.loadFromInternalMemory(false);

			// fill materials
			uint32_t materialsOffset = static_cast<uint32_t>(totalDrawSetsSize);
			Material_GL* materials = reinterpret_cast<Material_GL*>(modelData.get() + materialsOffset);
			fillMaterials(scene, materials, numMaterials);

			// fill scene graph
			uint32_t meshSceneOffset = static_cast<uint32_t>(totalDrawSetsSize + totalMaterialsSize);
			meshScene.childIndicesOffset = (meshScene.numNodes * sizeof(MeshSceneNode));
			meshScene.meshIndicesOffset  = meshScene.childIndicesOffset + (meshScene.numChildIndices * sizeof(uint32_t));
			meshScene.meshMetaDataOffset = meshScene.meshIndicesOffset + (meshScene.numMeshIndices * sizeof(uint32_t));
			meshScene.sceneNodes   = reinterpret_cast<MeshSceneNode*>(modelData.get() + meshSceneOffset);
			meshScene.childIndices = reinterpret_cast<uint32_t*>(modelData.get() + meshSceneOffset + meshScene.childIndicesOffset);
			meshScene.meshIndices  = reinterpret_cast<uint32_t*>(modelData.get() + meshSceneOffset + meshScene.meshIndicesOffset);
			meshScene.sceneNodeMetaData = reinterpret_cast<MeshSceneNodeMetaData*>(modelData.get() + meshSceneOffset + meshScene.meshMetaDataOffset);
			fillSceneGraph(scene, meshScene);

			// build the mesh object
			auto meshPtr = std::make_unique<Mesh_GL>(modelDataSize,
													 numMeshes,
													 numMaterials,
													 0, // drawSetsOffset, always 0 when using assimp import
													 materialsOffset,
													 meshSceneOffset,
													 std::move(modelData),
													 std::move(meshScene),
													 std::move(vb),
													 std::move(ib));
			
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

				assert(drawSet.numTexCoordChannels <= GRIFFIN_MAX_MATERIAL_TEXTURES && "too many uv channels in sub mesh");
				assert(drawSet.numColorChannels <= 8 && "too many vertex color channels in sub mesh");

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
		void fillMaterials(const aiScene& scene, Material_GL* materials,
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
				aiGetMaterialFloat(assimpMat, AI_MATKEY_SHININESS, &mat.shininess);

				aiColor4D color;
				aiGetMaterialColor(assimpMat, AI_MATKEY_COLOR_DIFFUSE, &color);
				mat.diffuseColor = { color.r, color.g, color.b };

				aiGetMaterialColor(assimpMat, AI_MATKEY_COLOR_AMBIENT, &color);
				mat.ambientColor = { color.r, color.g, color.b };

				aiGetMaterialColor(assimpMat, AI_MATKEY_COLOR_SPECULAR, &color);
				mat.specularColor = { color.r, color.g, color.b };

				aiGetMaterialColor(assimpMat, AI_MATKEY_COLOR_EMISSIVE, &color);
				mat.emissiveColor = { color.r, color.g, color.b };


				// TODO: set material flags and use the flags to determine ubershader key hash
				// maybe need to combine vertex flags for the vertex shader, material flags for the vertex/fragment shader
				// programs compiled on demand by the shader manager and maintained in lookup table

				// store the shader hash key in the material
				// how to handle LOD, e.g. turn off normal / displacement mapping with distance from camera

				// TODO: assimp defines texture stacks per texture type, and blend function between stack to come up with final color
				// Should we support stack of diffuse? I can't see a reason to support stack for any other texture type, but diffuse
				// might be useful for ambient occlusion and procedural effects like dirt and damage.

				//Texture Loading pseudo-code from http://assimp.sourceforge.net/lib_html/materials.html
				//Also see that page for example shader code to get color channel contributions

				//	have only one uv channel?
				//		assign channel 0 to all textures and break
				//	for all textures
				//		have uvwsrc for this texture?
				//			assign channel specified in uvwsrc
				//		else
				//			assign channels in ascending order for all texture stacks, 
				//				i.e. diffuse1 gets channel 1, opacity0 gets channel 0.

				// for each texture type
				uint32_t samplerIndex = 0;
				for (uint32_t tt = 0; tt < AI_TEXTURE_TYPE_MAX; ++tt) {
					// only support certain channels
					if (tt == aiTextureType_LIGHTMAP || tt == aiTextureType_DISPLACEMENT || tt == aiTextureType_UNKNOWN) {
						continue;
					}

					auto texType = MaterialTexture_None;
					switch (tt) {
						case aiTextureType_DIFFUSE:    texType = MaterialTexture_Diffuse; break;
						case aiTextureType_SPECULAR:   texType = MaterialTexture_Specular; break;
						case aiTextureType_AMBIENT:    texType = MaterialTexture_Ambient; break;
						case aiTextureType_EMISSIVE:   texType = MaterialTexture_Emissive; break;
						case aiTextureType_NORMALS:    texType = MaterialTexture_Normals; break;
						case aiTextureType_HEIGHT:     texType = MaterialTexture_Height; break;
						case aiTextureType_SHININESS:  texType = MaterialTexture_Shininess; break;
						case aiTextureType_REFLECTION: texType = MaterialTexture_Reflection; break;
						case aiTextureType_OPACITY:    texType = MaterialTexture_Opacity; break;
					}

					// for each texture of a type
					for (uint32_t i = 0; i < assimpMat->GetTextureCount((aiTextureType)tt); ++i) {
						// check for maximum samplers per material
						if (samplerIndex == GRIFFIN_MAX_MATERIAL_TEXTURES) {
							SDL_Log("Warning: more than %d textures in material, unsupported %u", GRIFFIN_MAX_MATERIAL_TEXTURES, tt);
							break;
						}
						// only support texture stack for diffuse channel
						if ((tt == aiTextureType_DIFFUSE && i > 3) || (tt != aiTextureType_DIFFUSE && i > 0)) {
							SDL_Log("Warning: trying to assign stack of texture type %u, index %u, unsupported", tt, i);
							break;
						}

						// find the UVW channel index for this texture
						int uvChannel = 0; // assign 0 if only one uv channel is preset in the mesh
						for (uint32_t mesh = 0; mesh < scene.mNumMeshes; ++mesh) {
							// find first mesh using this material, and with more than one UV channel
							if (scene.mMeshes[mesh]->mMaterialIndex == m &&
								scene.mMeshes[mesh]->GetNumUVChannels() > 1)
							{
								uvChannel = i; // default uv channel to the stack index
								assimpMat->Get(AI_MATKEY_UVWSRC(tt, i), uvChannel); // if the UVWSRC property exists, it will be filled
								break;
							}
						}
						mat.textures[samplerIndex].uvChannelIndex = static_cast<uint8_t>(uvChannel);
						
						// texture mapping mode (wrap, clamp, decal, mirror)
						aiTextureMapMode mappingModeU = aiTextureMapMode_Wrap;
						aiTextureMapMode mappingModeV = aiTextureMapMode_Wrap;
						assimpMat->Get(AI_MATKEY_MAPPINGMODE_U(tt, i), mappingModeU);
						assimpMat->Get(AI_MATKEY_MAPPINGMODE_V(tt, i), mappingModeU);
						mat.textures[samplerIndex].textureMappingModeU = (mappingModeU == aiTextureMapMode_Clamp ? MaterialTextureMappingMode_Clamp :
																		  (mappingModeU == aiTextureMapMode_Decal ? MaterialTextureMappingMode_Decal :
																		   (mappingModeU == aiTextureMapMode_Mirror ? MaterialTextureMappingMode_Mirror :
																		    MaterialTextureMappingMode_Wrap)));
						mat.textures[samplerIndex].textureMappingModeV = (mappingModeV == aiTextureMapMode_Clamp ? MaterialTextureMappingMode_Clamp :
																		  (mappingModeV == aiTextureMapMode_Decal ? MaterialTextureMappingMode_Decal :
																		   (mappingModeV == aiTextureMapMode_Mirror ? MaterialTextureMappingMode_Mirror :
																		    MaterialTextureMappingMode_Wrap)));

						// not supporting aiTextureFlags, aiTextureMapping, aiTextureOp

						// get texture filename
						aiString path;
						assimpMat->GetTexture((aiTextureType)tt, i, &path); // get the name, ignore other attributes for now

						if (path.length <= GRIFFIN_MAX_MATERIAL_TEXTURE_NAME_SIZE) {
							strcpy_s(mat.textures[samplerIndex].name, GRIFFIN_MAX_MATERIAL_TEXTURE_NAME_SIZE, path.C_Str());
						}
						else {
							SDL_Log("Warning: texture name length %d too long, \"%s\"", static_cast<int>(path.length), path.C_Str());
							break;
						}

						// do synchronous loading of texture since this is a utility function
						// if needed could also build the texture here and inject into cache, then pass assumeCached=true

						// everything converted successfully, set the texture type away from None
						mat.textures[samplerIndex].textureType = texType;
						++mat.numTextures;

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
				auto& thisMetaData = sceneGraph.sceneNodeMetaData[index];

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

				// populate mesh index array
				for (unsigned int m = 0; m < thisNode.numMeshes; ++m) {
					sceneGraph.meshIndices[meshIndexOffset + m] = assimpNode.mMeshes[m];
				}

				// copy metadata
				strcpy_s(thisMetaData.name, GRIFFIN_MAX_MESHSCENENODE_NAME_SIZE - 1, assimpNode.mName.C_Str());

				// advance the counters for the next node
				childIndexOffset += assimpNode.mNumChildren;
				meshIndexOffset += assimpNode.mNumMeshes;
			});
		}

	}
}

#endif