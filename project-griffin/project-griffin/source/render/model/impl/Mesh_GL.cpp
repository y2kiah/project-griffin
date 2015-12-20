/**
* @file Mesh_GL.cpp
* @author Jeff Kiah
*/
#include <render/model/Mesh_GL.h>
#include <application/Engine.h>
#include <GL/glew.h>
#include <utility>
#include <cassert>
#include <render/ShaderProgramLayouts_GL.h>
#include <render/Render.h>
#include <render/RenderResources.h>
#include <resource/ResourceLoader.h>
#include <utility/container/vector_queue.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define Meters_to_Feet 3.2808398950131233595800524934383f

namespace griffin {
	namespace render {
		using glm::dmat4;

		void Mesh_GL::bind(int drawSetIndex) const
		{
			glBindVertexArray(m_drawSets[drawSetIndex].glVAO);
		}


		void Mesh_GL::drawMesh(int drawSetIndex) const
		{
			assert(drawSetIndex >= 0 && drawSetIndex < static_cast<int>(m_numDrawSets) && "drawSetIndex out of range");
			bind(drawSetIndex);

			GLenum indexType = m_indexBuffer.getIndexType();
			DrawSet& drawSet = m_drawSets[drawSetIndex];

			glDrawRangeElements(drawSet.glPrimitiveType, drawSet.indexRangeStart, drawSet.indexRangeEnd,
								drawSet.numElements, indexType, reinterpret_cast<const GLvoid*>(drawSet.indexBaseOffset));

			//glDrawRangeElementsBaseVertex(/*GL_POINTS*/drawSet.glPrimitiveType, drawSet.indexRangeStart, drawSet.indexRangeEnd,
			//							  drawSet.numElements, indexType,
			//							  reinterpret_cast<const GLvoid*>(drawSet.indexBaseOffset), drawSet.vertexBaseOffset);
			//unbind(drawSetIndex);
		}


		void Mesh_GL::render(Engine& engine, uint8_t viewport,
							 int modelMatLoc, int modelViewMatLoc, int mvpMatLoc, int normalMatLoc,
							 int ambientLoc, int diffuseLoc, int specularLoc, int shininessLoc,
							 int diffuseMapLoc, float animTime,
							 const glm::dmat4& viewMat, const glm::mat4& projMat/*All TEMP*/) const
		{
			using namespace glm;
			
			auto& renderSystem = *engine.renderSystem;

			dmat4 modelToWorld;
			dmat4 nodeTransform;
			
			// temp
			modelToWorld = rotate(translate(modelToWorld, dvec3(0.0, -50.0, 0.0)),
								  radians(90.0),
								  dvec3(1.0, 0.0, 0.0));
			modelToWorld = scale(modelToWorld, dvec3(Meters_to_Feet));

			struct BFSQueueItem {
				uint32_t nodeIndex;
				dmat4    toWorld;
			};
			// TODO: lot of memory being created and destroyed every frame, should all be pre-calculated and stored for non-animated meshes?
			// Do this on mesh init and save a list of RenderEntries with matching index to the DrawSet
			// during the init, decompose the nodeTransform into separate translation, orientation, scale
			// Move this RenderEntry list to the Model_GL?
			vector_queue<BFSQueueItem> bfsQueue;
			bfsQueue.reserve(m_meshScene.numNodes);

			bfsQueue.push({ 0, modelToWorld }); // push root node to start traversal

			while (!bfsQueue.empty()) {
				auto& thisItem = bfsQueue.front();

				uint32_t nodeIndex = thisItem.nodeIndex;
				assert(nodeIndex >= 0 && nodeIndex < m_meshScene.numNodes && "node index out of range");

				const auto& node = m_meshScene.sceneNodes[thisItem.nodeIndex];
				nodeTransform = node.transform;

				// Get this node's animation, if any. Alter the node transform matrix.
				// TODO: make sure animation takes place only when instance is going to be visible after early frustum cull
				/*if (m_animations.numAnimationTracks > 0) {
					auto& anim = m_animations.animations[0];
					for (uint32_t na = anim.nodeAnimationsIndexOffset; na < anim.nodeAnimationsIndexOffset + anim.numNodeAnimations; ++na) {
						// TODO: move some of this out of the scene graph traversal? We know the active animations
						// ahead of time, do we need to check on every node?
						auto& nodeAnim = m_animations.nodeAnimations[na];
						if (nodeAnim.sceneNodeIndex == nodeIndex) {
							vec3 nodeTranslation;
							quat nodeRotation;
							vec3 nodeScale;

							// This code requires at least one key from each of position, rotation and scaling so we avoid having to
							// decompose the default node matrix. Luckily it appears that assimp always gives us at least one key for
							// each channel, but that could also be from Blender specifically. This assertion tells us if there is a
							// missing channel in the animation.
							assert(nodeAnim.numPositionKeys > 0 && nodeAnim.numRotationKeys > 0 && nodeAnim.numScalingKeys > 0 &&
								   "animation requires at least one key per channel");
							
							// Translation keyframes
							{
								int key1 = -1;
								int key2 = -1;
								// get nearest two key frames
								for (uint32_t pk = nodeAnim.positionKeysIndexOffset; pk < nodeAnim.positionKeysIndexOffset + nodeAnim.numPositionKeys; ++pk) {
									auto& posKey = m_animations.positionKeys[pk];
									if (animTime < posKey.time) {
										key1 = (pk == nodeAnim.positionKeysIndexOffset ? pk : pk - 1);
										key2 = pk;
										break;
									}
								}
								// went past the last key
								if (key1 == -1) {
									key1 = nodeAnim.positionKeysIndexOffset + nodeAnim.numPositionKeys - 1;
									key2 = key1;
								}

								// TODO: look at pre/post state, we may be able to exit early and accept the default modelToWorld when key1 == key2, depending on the state
								// Also, the key1 or key2 at either end of the animation may have to be set to default node transform instead of clamping the animations frame
								float time1 = m_animations.positionKeys[key1].time;
								vec3 pos1(m_animations.positionKeys[key1].x, m_animations.positionKeys[key1].y, m_animations.positionKeys[key1].z);
								float time2 = m_animations.positionKeys[key2].time;
								vec3 pos2(m_animations.positionKeys[key2].x, m_animations.positionKeys[key2].y, m_animations.positionKeys[key2].z);
								
								float interp = 0.0f;
								if (key1 != key2) {
									interp = (animTime - time1) / (time2 - time1);
								}

								// TODO: allow interpolation curves other than linear... hermite, cubic, spring system, etc.
								nodeTranslation = mix(pos1, pos2, interp);
							}
							// Rotation keyframes
							{
								int key1 = -1;
								int key2 = -1;

								// get nearest two key frames
								for (uint32_t k = nodeAnim.rotationKeysIndexOffset; k < nodeAnim.rotationKeysIndexOffset + nodeAnim.numRotationKeys; ++k) {
									auto& rotKey = m_animations.rotationKeys[k];
									if (animTime < rotKey.time) {
										key1 = (k == nodeAnim.rotationKeysIndexOffset ? k : k - 1);
										key2 = k;
										break;
									}
								}

								// went past the last key
								if (key1 == -1) {
									key1 = nodeAnim.rotationKeysIndexOffset + nodeAnim.numRotationKeys - 1;
									key2 = key1;
								}

								float time1 = m_animations.rotationKeys[key1].time;
								quat rot1(m_animations.rotationKeys[key1].w, m_animations.rotationKeys[key1].x, m_animations.rotationKeys[key1].y, m_animations.rotationKeys[key1].z);
								float time2 = m_animations.rotationKeys[key2].time;
								quat rot2(m_animations.rotationKeys[key2].w, m_animations.rotationKeys[key2].x, m_animations.rotationKeys[key2].y, m_animations.rotationKeys[key2].z);

								float interp = 0.0f;
								if (key1 != key2) {
									interp = (animTime - time1) / (time2 - time1);
								}

								nodeRotation = slerp(rot1, rot2, interp);
							}
							// Scaling keyframes
							{
								int key1 = -1;
								int key2 = -1;

								// get nearest two key frames
								for (uint32_t k = nodeAnim.scalingKeysIndexOffset; k < nodeAnim.scalingKeysIndexOffset + nodeAnim.numScalingKeys; ++k) {
									auto& scaleKey = m_animations.scalingKeys[k];
									if (animTime < scaleKey.time) {
										key1 = (k == nodeAnim.scalingKeysIndexOffset ? k : k - 1);
										key2 = k;
										break;
									}
								}

								// went past the last key
								if (key1 == -1) {
									key1 = nodeAnim.scalingKeysIndexOffset + nodeAnim.numScalingKeys - 1;
									key2 = key1;
								}

								float time1 = m_animations.scalingKeys[key1].time;
								vec3 scale1(m_animations.scalingKeys[key1].x, m_animations.scalingKeys[key1].y, m_animations.scalingKeys[key1].z);
								float time2 = m_animations.scalingKeys[key2].time;
								vec3 scale2(m_animations.scalingKeys[key2].x, m_animations.scalingKeys[key2].y, m_animations.scalingKeys[key2].z);

								float interp = 0.0f;
								if (key1 != key2) {
									interp = (animTime - time1) / (time2 - time1);
								}

								nodeScale = mix(scale1, scale2, interp);
							}
							nodeTransform = mat4_cast(nodeRotation);
							nodeTransform[3].xyz = nodeTranslation;
							nodeTransform = scale(nodeTransform, dvec3(nodeScale));
						}
					}
				}*/

				modelToWorld = thisItem.toWorld * nodeTransform;
				
				// original code, jitters far from origin
				//mat4 modelView(viewMat * modelToWorld);
				//mat4 mvp(projMat * modelView);
				//mat4 normalMat(transpose(inverse(mat3(modelView))));

				// transform world space to eye space on CPU in double precision, then send single to GPU
				// see http://blogs.agi.com/insight3d/index.php/2008/09/03/precisions-precisions/
				dmat4 modelViewWorld(viewMat * modelToWorld);
				
				dvec4 nodeTranslationWorld(modelToWorld[0][3], modelToWorld[1][3], modelToWorld[2][3], 1.0);
				vec3 nodeTranslationEye(nodeTranslationWorld * modelViewWorld);

				mat4 modelViewEye(modelViewWorld);
				modelViewEye[0][3] = nodeTranslationEye.x;
				modelViewEye[1][3] = nodeTranslationEye.y;
				modelViewEye[2][3] = nodeTranslationEye.z;

				mat4 mvp(projMat * modelViewEye);
				mat4 normalMat(transpose(inverse(mat3(modelViewEye))));

				glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, &mat4(modelToWorld)[0][0]);
				glUniformMatrix4fv(modelViewMatLoc, 1, GL_FALSE, &modelViewEye[0][0]);
				glUniformMatrix4fv(mvpMatLoc, 1, GL_FALSE, &mvp[0][0]);
				glUniformMatrix4fv(normalMatLoc, 1, GL_FALSE, &normalMat[0][0]);

				// draw this node's meshes
				for (uint32_t m = 0; m < node.numMeshes; ++m) {
					uint32_t ds = m_meshScene.meshIndices[node.meshIndexOffset + m];
					auto& drawSet = m_drawSets[ds];
					Material_GL& mat = m_materials[drawSet.materialIndex];

					glUniform3fv(ambientLoc, 1, &mat.ambientColor[0]);
					glUniform3fv(diffuseLoc, 1, &mat.diffuseColor[0]);
					glUniform3fv(specularLoc, 1, &mat.specularColor[0]);
					glUniform1f(shininessLoc, mat.shininess);
					
					// renderer should do this as part of the render key sort/render, not the mesh
					// TEMP, assuming one texture
					if (mat.numTextures > 0) {
						// should NOT use this method to get the resource, it serializes to the worker thread
						// this part of the render is a time-critical section, should have the resourcePtr directly by now
						// store resourcePtr's within the model containing this mesh, render from the model file
						auto tex = g_resourceLoader.lock()->getResource<Texture2D_GL>(mat.textures[0].textureResourceHandle, CacheType::Cache_Materials);
						if (tex) {
							tex->bind(GL_TEXTURE4);
							glUniform1i(diffuseMapLoc, 4);
						}
					}

					RenderQueueKey key{};
					RenderEntry entry{
						nodeTranslationWorld,

					};
					renderSystem.addRenderEntry(viewport, key, std::move(entry));
					
					// TEMP
					drawMesh(ds);
				}

				// push children to traverse
				for (uint32_t c = 0; c < node.numChildren; ++c) {
					uint32_t childNodeIndex = m_meshScene.childIndices[node.childIndexOffset + c];

					assert(childNodeIndex >= 0 && childNodeIndex < m_meshScene.numNodes && "child node index out of range");
					assert(childNodeIndex > nodeIndex && "child node is not lower in the tree");

					bfsQueue.push({ childNodeIndex, modelToWorld });
				}

				bfsQueue.pop();
			}
		}


		void Mesh_GL::initVAO(int drawSetIndex) const
		{
			assert(drawSetIndex >= 0 && drawSetIndex < static_cast<int>(m_numDrawSets) && "drawSetIndex out of range");

			auto& drawSet = m_drawSets[drawSetIndex];

			glGenVertexArrays(1, &drawSet.glVAO);
			glBindVertexArray(m_drawSets[drawSetIndex].glVAO);

			m_vertexBuffer.bind();
			m_indexBuffer.bind();

			if (drawSet.vertexFlags & Vertex_Positions) {
				glEnableVertexAttribArray(VertexLayout_Position);
				glVertexAttribPointer(
					VertexLayout_Position,
					3,                         // size
					GL_FLOAT,                  // type
					GL_FALSE,                  // normalize fixed-point?
					drawSet.vertexSize,        // stride
					reinterpret_cast<void*>(drawSet.vertexBaseOffset) // array buffer offset
					);
			}

			if (drawSet.vertexFlags & Vertex_Normals) {
				glEnableVertexAttribArray(VertexLayout_Normal);
				glVertexAttribPointer(
					VertexLayout_Normal,
					3, GL_FLOAT, GL_FALSE,
					drawSet.vertexSize,
					reinterpret_cast<void*>(drawSet.vertexBaseOffset + drawSet.normalOffset)
					);
			}

			if (drawSet.vertexFlags & Vertex_TangentsAndBitangents) {
				glEnableVertexAttribArray(VertexLayout_Tangent);
				glEnableVertexAttribArray(VertexLayout_Bitangent);
				glVertexAttribPointer(
					VertexLayout_Tangent,
					3, GL_FLOAT, GL_FALSE,
					drawSet.vertexSize,
					reinterpret_cast<void*>(drawSet.vertexBaseOffset + drawSet.tangentOffset)
					);
				glVertexAttribPointer(
					VertexLayout_Bitangent,
					3, GL_FLOAT, GL_FALSE,
					drawSet.vertexSize,
					reinterpret_cast<void*>(drawSet.vertexBaseOffset + drawSet.bitangentOffset)
					);
			}

			if (drawSet.vertexFlags & Vertex_TextureCoords) {
				unsigned int offset = drawSet.vertexBaseOffset + drawSet.texCoordsOffset;

				for (int c = 0; c < drawSet.numTexCoordChannels; ++c) {
					glEnableVertexAttribArray(VertexLayout_TextureCoords + c);
					glVertexAttribPointer(
						VertexLayout_TextureCoords + c,
						drawSet.numTexCoordComponents[c],
						GL_FLOAT, GL_FALSE,
						drawSet.vertexSize,
						reinterpret_cast<void*>(offset)
						);

					offset += (sizeof(float) * drawSet.numTexCoordComponents[c]);
				}
			}

			if (drawSet.vertexFlags & Vertex_Colors) {
				unsigned int offset = drawSet.vertexBaseOffset + drawSet.colorsOffset;

				for (int c = 0; c < drawSet.numColorChannels; ++c) {
					glEnableVertexAttribArray(VertexLayout_Colors + c);
					glVertexAttribPointer(
						VertexLayout_Colors + c,
						4, GL_FLOAT, GL_FALSE,
						drawSet.vertexSize,
						reinterpret_cast<void*>(offset)
						);

					offset += (sizeof(float) * 4);
				}
			}
		}


		void Mesh_GL::initRenderEntries()
		{
			struct BFSQueueItem {
				uint32_t nodeIndex;
				dmat4    toWorld;
			};

			vector_queue<BFSQueueItem> bfsQueue;
			bfsQueue.reserve(m_meshScene.numNodes);

			bfsQueue.push({ 0, modelToWorld }); // push root node to start traversal

			while (!bfsQueue.empty()) {
				auto& thisItem = bfsQueue.front();

				uint32_t nodeIndex = thisItem.nodeIndex;
				assert(nodeIndex >= 0 && nodeIndex < m_meshScene.numNodes && "node index out of range");

				const auto& node = m_meshScene.sceneNodes[thisItem.nodeIndex];
				nodeTransform = node.transform;

				modelToWorld = thisItem.toWorld * nodeTransform;

				// draw this node's meshes
				for (uint32_t m = 0; m < node.numMeshes; ++m) {
					uint32_t ds = m_meshScene.meshIndices[node.meshIndexOffset + m];
					auto& drawSet = m_drawSets[ds];
					Material_GL& mat = m_materials[drawSet.materialIndex];

					// renderer should do this as part of the render key sort/render, not the mesh
					// TEMP, assuming one texture
					if (mat.numTextures > 0) {
						// should NOT use this method to get the resource, it serializes to the worker thread
						// this part of the render is a time-critical section, should have the resourcePtr directly by now
						// store resourcePtr's within the model containing this mesh, render from the model file
						auto tex = g_resourceLoader.lock()->getResource<Texture2D_GL>(mat.textures[0].textureResourceHandle, CacheType::Cache_Materials);
						if (tex) {
							tex->bind(GL_TEXTURE4);
							glUniform1i(diffuseMapLoc, 4);
						}
					}

					RenderQueueKey key{};
					RenderEntry entry{
						nodeTranslationWorld,

					};
					renderSystem.addRenderEntry(viewport, key, std::move(entry));

					// TEMP
					drawMesh(ds);
				}

				// push children to traverse
				for (uint32_t c = 0; c < node.numChildren; ++c) {
					uint32_t childNodeIndex = m_meshScene.childIndices[node.childIndexOffset + c];

					assert(childNodeIndex >= 0 && childNodeIndex < m_meshScene.numNodes && "child node index out of range");
					assert(childNodeIndex > nodeIndex && "child node is not lower in the tree");

					bfsQueue.push({ childNodeIndex, modelToWorld });
				}

				bfsQueue.pop();
			}
		}


		void Mesh_GL::unbind(int drawSetIndex) const
		{
			assert(drawSetIndex >= 0 && drawSetIndex < static_cast<int>(m_numDrawSets) && "drawSetIndex out of range");

			glBindVertexArray(0); // break the existing vertex array object binding

			/*auto& drawSet = m_drawSets[drawSetIndex];

			if (drawSet.vertexFlags & Vertex_Positions) {
			glDisableVertexAttribArray(VertexLayout_Position);
			}

			if (drawSet.vertexFlags & Vertex_Normals) {
			glDisableVertexAttribArray(VertexLayout_Normal);
			}

			if (drawSet.vertexFlags & Vertex_TangentsAndBitangents) {
			glDisableVertexAttribArray(VertexLayout_Tangent);
			glDisableVertexAttribArray(VertexLayout_Bitangent);
			}

			if (drawSet.vertexFlags & Vertex_TextureCoords) {
			for (int c = 0; c < drawSet.numTexCoordChannels; ++c) {
			glDisableVertexAttribArray(VertexLayout_TextureCoords + c);
			}
			}

			if (drawSet.vertexFlags & Vertex_Colors) {
			for (int c = 0; c < drawSet.numColorChannels; ++c) {
			glDisableVertexAttribArray(VertexLayout_Colors + c);
			}
			}*/
		}


		// Animation Functions

		uint32_t Mesh_GL::getAnimationTrackIndexByName(const char* name) const
		{
			for (uint32_t a = 0; a < m_animations.numAnimationTracks; ++a) {
				if (strncmp(m_animations.trackNames[a].name,
							name, MAX_ANIMATION_NAME_SIZE) == 0)
				{
					return a;
				}
			}
			return UINT32_MAX;
		}


		// Serialization Functions

		#define GRIFFIN_MESH_SERIALIZATION_CURRENT_VERSION	3

		/**
		* Mesh_GL binary file header, contains all properties needed for serialization
		*/
		struct Mesh_GL_Header {
			uint8_t		key[3];						//<! always {'g','m','d'}
			uint8_t		version;					//<! file version
			uint32_t	bufferSize;

			uint32_t	numDrawSets;
			uint32_t	numMaterials;
			uint32_t	drawSetsOffset;
			uint32_t	materialsOffset;
			uint32_t	meshSceneOffset;
			
			uint32_t	sceneNumNodes;
			uint32_t	sceneNumChildIndices;
			uint32_t	sceneNumMeshIndices;
			uint32_t	sceneChildIndicesOffset;
			uint32_t	sceneMeshIndicesOffset;
			uint32_t	sceneMetaDataOffset;
			
			uint32_t	animationsSize;
			uint32_t	animationsOffset;
			uint32_t	numAnimationTracks;
			uint32_t	nodeAnimationsOffset;
			uint32_t	positionKeysOffset;
			uint32_t	rotationKeysOffset;
			uint32_t	scalingKeysOffset;
			uint32_t	animationTrackNamesOffset;

			uint32_t	vertexBufferSize;
			uint32_t	vertexBufferOffset;
			
			uint32_t	indexBufferSize;
			uint32_t	indexBufferOffset;
			uint8_t		indexBufferFlags;

			uint8_t		_padding_end[3];			//<! pad for 8-byte alignment of following struct in buffer
		};
		static_assert(sizeof(Mesh_GL_Header) % 8 == 0, "Mesh_GL_Header size should be multiple of 8 for alignment of mesh buffer");
		static_assert(std::is_trivially_copyable<Mesh_GL_Header>::value, "Mesh_GL_Header must be trivially copyable for serialization");

		void Mesh_GL::serialize(std::ostream& out)
		{
			uint32_t headerSize = sizeof(Mesh_GL_Header);
			uint32_t drawSetsSize = m_numDrawSets * sizeof(DrawSet);
			uint32_t materialsSize = m_numMaterials * sizeof(Material_GL);
			uint32_t sceneNodesSize = m_meshScene.numNodes * sizeof(MeshSceneNode);
			uint32_t sceneChildIndicesSize = m_meshScene.numChildIndices * sizeof(uint32_t);
			uint32_t sceneMeshIndicesSize = m_meshScene.numMeshIndices * sizeof(uint32_t);
			uint32_t sceneMetaDataSize = m_meshScene.numNodes * sizeof(MeshSceneNodeMetaData);

			// get the total size of the model buffer
			uint32_t bufferSize = drawSetsSize + materialsSize + sceneNodesSize +
				sceneChildIndicesSize + sceneMeshIndicesSize +
				sceneMetaDataSize + m_animationsSize +
				static_cast<uint32_t>(m_vertexBuffer.getSize()) +
				static_cast<uint32_t>(m_indexBuffer.getSize());
				
			// build the header containing sizes and offsets
			Mesh_GL_Header header = {}; // zero-init the header
			header.key[0] = 'g'; header.key[1] = 'm'; header.key[2] = 'd';
			header.version					= GRIFFIN_MESH_SERIALIZATION_CURRENT_VERSION;
			header.bufferSize				= bufferSize;
			
			header.numDrawSets				= m_numDrawSets;
			header.numMaterials				= m_numMaterials;
			header.drawSetsOffset			= headerSize + 0;
			header.materialsOffset			= header.drawSetsOffset + drawSetsSize;
			header.meshSceneOffset			= header.materialsOffset + materialsSize;
			
			header.sceneNumNodes			= m_meshScene.numNodes;
			header.sceneNumChildIndices		= m_meshScene.numChildIndices;
			header.sceneNumMeshIndices		= m_meshScene.numMeshIndices;
			header.sceneChildIndicesOffset	= header.meshSceneOffset + sceneNodesSize;
			header.sceneMeshIndicesOffset	= header.sceneChildIndicesOffset + sceneChildIndicesSize;
			header.sceneMetaDataOffset		= header.sceneMeshIndicesOffset + sceneMeshIndicesSize;
			
			header.animationsSize			= m_animationsSize;
			header.animationsOffset			= header.sceneMetaDataOffset + sceneMetaDataSize;
			header.numAnimationTracks		= m_animations.numAnimationTracks;
			header.nodeAnimationsOffset		= m_animations.nodeAnimationsOffset;
			header.positionKeysOffset		= m_animations.positionKeysOffset;
			header.rotationKeysOffset		= m_animations.rotationKeysOffset;
			header.scalingKeysOffset		= m_animations.scalingKeysOffset;
			header.animationTrackNamesOffset = m_animations.trackNamesOffset;

			header.vertexBufferSize			= static_cast<uint32_t>(m_vertexBuffer.getSize());
			header.vertexBufferOffset		= header.animationsOffset + m_animationsSize;
			
			header.indexBufferSize			= static_cast<uint32_t>(m_indexBuffer.getSize());
			header.indexBufferOffset		= header.vertexBufferOffset + header.vertexBufferSize;
			header.indexBufferFlags			= static_cast<uint8_t>(m_indexBuffer.getFlags());

			// write header
			out.write(reinterpret_cast<const char*>(&header), headerSize);

			// write data buffers
			out.write(reinterpret_cast<const char*>(m_drawSets), drawSetsSize);
			out.write(reinterpret_cast<const char*>(m_materials), materialsSize);
			out.write(reinterpret_cast<const char*>(m_meshScene.sceneNodes), sceneNodesSize);
			out.write(reinterpret_cast<const char*>(m_meshScene.childIndices), sceneChildIndicesSize);
			out.write(reinterpret_cast<const char*>(m_meshScene.meshIndices), sceneMeshIndicesSize);
			out.write(reinterpret_cast<const char*>(m_meshScene.sceneNodeMetaData), sceneMetaDataSize);
			out.write(reinterpret_cast<const char*>(m_animations.animations), m_animationsSize);

			// source the vertex data from either the m_modelData buffer, or the VertexBuffer_GL's internal buffer,
			// it could be one or the other depending on whether the model was deserialized or imported
			unsigned char* vertexData = nullptr;
			if (m_vertexBufferOffset != 0) {
				vertexData = m_modelData.get() + m_vertexBufferOffset;
			}
			else {
				vertexData = m_vertexBuffer.data();
			}
			assert(vertexData != nullptr && "vertex buffer data not available");
			out.write(reinterpret_cast<const char*>(vertexData), header.vertexBufferSize);
			
			unsigned char* indexData = nullptr;
			if (m_indexBufferOffset != 0) {
				indexData = m_modelData.get() + m_indexBufferOffset;
			}
			else {
				indexData = m_indexBuffer.data();
			}
			assert(indexData != nullptr && "index buffer data not available");
			out.write(reinterpret_cast<const char*>(indexData), header.indexBufferSize);
		}


		void Mesh_GL::deserialize(std::istream& in)
		{
			// read the header
			Mesh_GL_Header header = {};
			size_t headerSize = sizeof(Mesh_GL_Header);
			in.read(reinterpret_cast<char*>(&header), headerSize);

			// total size of the model data buffer, we are matching how the resource loader works
			size_t totalSize = headerSize + header.bufferSize;
			m_modelData = std::make_unique<unsigned char[]>(totalSize);

			// copy the header into the modelData buffer (this is how it would be using the resource loader)
			memcpy_s(m_modelData.get(), headerSize, &header, headerSize);

			// read the model data buffer
			in.read(reinterpret_cast<char*>(m_modelData.get() + headerSize), header.bufferSize);

			loadFromInternalMemory();
		}


		void Mesh_GL::loadFromInternalMemory()
		{
			// the header exists at the beginning of the modelData buffer
			Mesh_GL_Header& header = *reinterpret_cast<Mesh_GL_Header*>(m_modelData.get());
			uint32_t totalSize = sizeof(Mesh_GL_Header) + header.bufferSize;

			// do some sanity checks
			if (header.key[0] != 'g' || header.key[1] != 'm' || header.key[2] != 'd' ||
				header.version != GRIFFIN_MESH_SERIALIZATION_CURRENT_VERSION)
			{
				throw std::logic_error("Unrecognized file format");
			}

			if (header.drawSetsOffset != sizeof(Mesh_GL_Header) ||
				header.materialsOffset != header.drawSetsOffset + (header.numDrawSets * sizeof(DrawSet)) ||
				header.meshSceneOffset != header.materialsOffset + (header.numMaterials * sizeof(Material_GL)) ||
				header.sceneChildIndicesOffset != header.meshSceneOffset + (header.sceneNumNodes * sizeof(MeshSceneNode)) ||
				header.sceneMeshIndicesOffset != header.sceneChildIndicesOffset + (header.sceneNumChildIndices * sizeof(uint32_t)) ||
				header.sceneMetaDataOffset != header.sceneMeshIndicesOffset + (header.sceneNumMeshIndices * sizeof(uint32_t)) ||
				header.animationsOffset != header.sceneMetaDataOffset + (header.sceneNumNodes * sizeof(MeshSceneNodeMetaData)) ||
				header.vertexBufferOffset != header.animationsOffset + header.animationsSize ||
				header.indexBufferOffset != header.vertexBufferOffset + header.vertexBufferSize ||
				totalSize != header.indexBufferOffset + header.indexBufferSize)
			{
				throw std::logic_error("File format deserialization error");
			}

			// copy properties from header to member vars
			m_sizeBytes						= totalSize;
			m_numDrawSets					= header.numDrawSets;
			m_numMaterials					= header.numMaterials;
			m_drawSetsOffset				= header.drawSetsOffset;
			m_materialsOffset				= header.materialsOffset;
			m_meshSceneOffset				= header.meshSceneOffset;
			m_animationsSize				= header.animationsSize;
			m_animationsOffset				= header.animationsOffset;
			m_vertexBufferOffset			= header.vertexBufferOffset;
			m_indexBufferOffset				= header.indexBufferOffset;
			
			m_meshScene.numNodes			= header.sceneNumNodes;
			m_meshScene.numChildIndices		= header.sceneNumChildIndices;
			m_meshScene.numMeshIndices		= header.sceneNumMeshIndices;
			m_meshScene.childIndicesOffset	= header.sceneChildIndicesOffset;
			m_meshScene.meshIndicesOffset	= header.sceneMeshIndicesOffset;
			m_meshScene.meshMetaDataOffset	= header.sceneMetaDataOffset;
			
			m_animations.numAnimationTracks	= header.numAnimationTracks;
			m_animations.nodeAnimationsOffset = header.nodeAnimationsOffset;
			m_animations.positionKeysOffset	= header.positionKeysOffset;
			m_animations.rotationKeysOffset	= header.rotationKeysOffset;
			m_animations.scalingKeysOffset	= header.scalingKeysOffset;
			m_animations.trackNamesOffset	= header.animationTrackNamesOffset;

			// set size and flags into the buffers, we later call loadFromMemory and use getters to read these back
			m_vertexBuffer.set(nullptr, header.vertexBufferSize);
			m_indexBuffer.set(nullptr,  header.indexBufferSize, static_cast<IndexBuffer_GL::IndexBufferFlags>(header.indexBufferFlags));

			// fix up internal pointers based on offsets in header
			m_drawSets						= reinterpret_cast<DrawSet*>(m_modelData.get() + m_drawSetsOffset);
			m_materials						= reinterpret_cast<Material_GL*>(m_modelData.get() + m_materialsOffset);
			m_meshScene.sceneNodes			= reinterpret_cast<MeshSceneNode*>(m_modelData.get() + m_meshSceneOffset);
			m_meshScene.childIndices		= reinterpret_cast<uint32_t*>(m_modelData.get() + m_meshScene.childIndicesOffset);
			m_meshScene.meshIndices			= reinterpret_cast<uint32_t*>(m_modelData.get() + m_meshScene.meshIndicesOffset);
			m_meshScene.sceneNodeMetaData	= reinterpret_cast<MeshSceneNodeMetaData*>(m_modelData.get() + m_meshScene.meshMetaDataOffset);

			m_animations.animations			= reinterpret_cast<AnimationTrack*>(m_modelData.get() + m_animationsOffset);
			m_animations.nodeAnimations		= reinterpret_cast<NodeAnimation*>(m_modelData.get() + m_animationsOffset + m_animations.nodeAnimationsOffset);
			m_animations.positionKeys		= reinterpret_cast<PositionKeyFrame*>(m_modelData.get() + m_animationsOffset + m_animations.positionKeysOffset);
			m_animations.rotationKeys		= reinterpret_cast<RotationKeyFrame*>(m_modelData.get() + m_animationsOffset + m_animations.rotationKeysOffset);
			m_animations.scalingKeys		= reinterpret_cast<ScalingKeyFrame*>(m_modelData.get() + m_animationsOffset + m_animations.scalingKeysOffset);
			m_animations.trackNames			= reinterpret_cast<AnimationTrackMetaData*>(m_modelData.get() + m_animationsOffset + m_animations.trackNamesOffset);
		}

		
		void Mesh_GL::createResourcesFromInternalMemory(const std::wstring& filePath)
		{
			// This function is called by the deserialization / resource loading routines, not by
			// the assimp import. The size/flags of the buffers are set in loadFromInternalMemory.

			m_vertexBuffer.loadFromMemory(m_modelData.get() + m_vertexBufferOffset,
										  m_vertexBuffer.getSize());

			m_indexBuffer.loadFromMemory(m_modelData.get() + m_indexBufferOffset,
										 m_indexBuffer.getSize(),
										 IndexBuffer_GL::getSizeOfElement(m_indexBuffer.getFlags()));

			initVAOs();

			// materials
			for (uint32_t m = 0; m < m_numMaterials; ++m) {
				auto& mat = m_materials[m];
				for (uint32_t t = 0; t < mat.numTextures; ++t) {
					auto& tex = mat.textures[t];
					if (tex.textureType != MaterialTexture_None) {
						// convert from ascii to wide character set
						string aName(tex.name);
						wstring wName;
						wName.assign(aName.begin(), aName.end());
						
						// prefix texture path with the path to the model being loaded
						wName = filePath.substr(0, filePath.find_last_of(L'/')) + L'/' + wName;
						SDL_Log("trying to load %s", aName.assign(wName.begin(),wName.end()).c_str());

						auto resHandle = render::loadTexture2D(wName, resource::CacheType::Cache_Materials);
						// TEMP, blocking call, need to make this async, use task system
						// BUT, the continuation must update this handle, assuming "this" pointer is captured by reference,
						// the material may move in memory, since the resource system is free to move it, potential bug
						// use the resource id to look up by handle to get its current memory location from the task
						tex.textureResourceHandle = resHandle.handle();
						auto pTex = render::g_resourceLoader.lock()->getResource<Texture2D_GL>(tex.textureResourceHandle, resource::CacheType::Cache_Materials);
						pTex->bind(GL_TEXTURE0);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					}
				}
			}
		}


		// Constructors / Destructor

		Mesh_GL::Mesh_GL(ByteBuffer data, size_t size) :
			m_modelData(std::move(data)),
			m_sizeBytes{ static_cast<uint32_t>(size) }
		{
			loadFromInternalMemory();
		}


		Mesh_GL::Mesh_GL(size_t sizeBytes, uint16_t numDrawSets, uint16_t numMaterials,
						 uint32_t drawSetsOffset, uint32_t materialsOffset, uint32_t meshSceneOffset,
						 uint32_t animationsSize, uint32_t animationsOffset,
						 ByteBuffer modelData,
						 MeshSceneGraph&& meshScene,
						 MeshAnimations&& meshAnimations,
						 VertexBuffer_GL&& vb, IndexBuffer_GL&& ib) :
			m_sizeBytes{ static_cast<uint32_t>(sizeBytes) },
			m_numDrawSets{ numDrawSets },
			m_numMaterials{ numMaterials },
			m_drawSetsOffset{ drawSetsOffset },
			m_materialsOffset{ materialsOffset },
			m_meshSceneOffset{ meshSceneOffset },
			m_animationsSize{ animationsSize },
			m_animationsOffset{ animationsOffset },
			m_vertexBufferOffset{ 0 },
			m_indexBufferOffset{ 0 },
			m_modelData(std::move(modelData)),
			m_meshScene(std::forward<MeshSceneGraph>(meshScene)),
			m_animations(std::forward<MeshAnimations>(meshAnimations)),
			m_vertexBuffer(std::forward<VertexBuffer_GL>(vb)),
			m_indexBuffer(std::forward<IndexBuffer_GL>(ib))
		{
			// fix up the internal pointers into m_modelData
			m_drawSets = reinterpret_cast<DrawSet*>(m_modelData.get() + m_drawSetsOffset);
			m_materials = reinterpret_cast<Material_GL*>(m_modelData.get() + m_materialsOffset);

			initVAOs();
		}


		Mesh_GL::Mesh_GL(Mesh_GL&& other) :
			m_numDrawSets{ other.m_numDrawSets },
			m_numMaterials{ other.m_numMaterials },
			m_drawSetsOffset{ other.m_drawSetsOffset },
			m_materialsOffset{ other.m_materialsOffset },
			m_meshSceneOffset{ other.m_meshSceneOffset },
			m_animationsSize{ other.m_animationsSize },
			m_animationsOffset{ other.m_animationsOffset },
			m_vertexBufferOffset{ other.m_vertexBufferOffset },
			m_indexBufferOffset{ other.m_indexBufferOffset },
			m_drawSets{ other.m_drawSets },
			m_materials{ other.m_materials },
			m_meshScene(std::move(other.m_meshScene)),
			m_animations(std::move(other.m_animations)),
			m_vertexBuffer(std::move(other.m_vertexBuffer)),
			m_indexBuffer(std::move(other.m_indexBuffer)),
			m_sizeBytes{ other.m_sizeBytes },
			m_modelData(std::move(other.m_modelData))
		{
			other.m_numDrawSets = 0;
			other.m_numMaterials = 0;
			other.m_drawSetsOffset = 0;
			other.m_materialsOffset = 0;
			other.m_meshSceneOffset = 0;
			other.m_animationsSize = 0;
			other.m_animationsOffset = 0;
			other.m_vertexBufferOffset = 0;
			other.m_indexBufferOffset = 0;
			other.m_drawSets = nullptr;
			other.m_materials = nullptr;
			other.m_sizeBytes = 0;
		}


		Mesh_GL::~Mesh_GL() {
			// delete the VAO objects
			for (uint32_t ds = 0; ds < m_numDrawSets; ++ds) {
				glDeleteVertexArrays(1, &m_drawSets[ds].glVAO);
			}
		}


		void skinning() {
			//Parameters

			const glm::mat4 * poseMatrix = nullptr;
			uint8_t const * vertexData = nullptr;
			size_t posStride = 0;
			size_t blendIndexStride = 0;
			size_t blendWeightStride = 0;
			size_t numVertices = 0;
			size_t bytesPerVertex = 0;
			int numWeightsPerVertex = 0;

			glm::vec3 max(-std::numeric_limits<float>::max());
			glm::vec3 min(std::numeric_limits<float>::min());

			for (size_t i = 0; i < numVertices; ++i) {
				const glm::vec3 *vPos = reinterpret_cast<const glm::vec3 *>(vertexData + posStride);
				const uint8_t *blendIndex = reinterpret_cast<const uint8_t *>(vertexData + blendIndexStride);
				const float *blendWeight = reinterpret_cast<const float *>(vertexData + blendWeightStride);

				for (int j = 0; j < numWeightsPerVertex; ++j) {
					const glm::vec4 translatedPos = poseMatrix[blendIndex[j]] * glm::vec4(*vPos, 1.0f);

					max.x = std::max(max.x, translatedPos.x);
					max.y = std::max(max.y, translatedPos.y);
					max.z = std::max(max.z, translatedPos.z);
					min.x = std::min(min.x, translatedPos.x);
					min.y = std::min(min.y, translatedPos.y);
					min.z = std::min(min.z, translatedPos.z);
				}
				vertexData += bytesPerVertex;
			}
		}
	}
}