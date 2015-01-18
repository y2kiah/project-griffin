#ifdef GRIFFIN_TOOLS_BUILD
#include "../ModelImport_Assimp.h"

#pragma comment( lib, "assimp64.lib" )

#include <assimp/Importer.hpp>	// C++ importer interface
#include <assimp/scene.h>		// Output data structure
#include <assimp/postprocess.h>	// Post processing flags
#include <assimp/config.h>		// Configuration properties

#include <string>
#include <memory>

using namespace Assimp;
using std::string;
using std::wstring;
using std::unique_ptr;


/*---------------------------------------------------------------------
Import a 3d model using Assimp
---------------------------------------------------------------------*/
bool importModelFile(const string &filename)
{
	Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);

	uint32_t ppFlags = /*aiProcess_ConvertToLeftHanded |*/
		aiProcess_TransformUVCoords |
		aiProcessPreset_TargetRealtime_MaxQuality;

	const aiScene *scene = importer.ReadFile(filename, ppFlags);

	if (!scene) {
		//debugPrintf("Mesh::importFromFile: failed: %s\n", importer.GetErrorString());
		return false;
	}

	// get Materials
	m_materials.reserve(scene->mNumMaterials);
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

	// get Meshes
	m_meshes.reserve(scene->mNumMeshes);
	for (uint32_t m = 0; m < scene->mNumMeshes; ++m) {
		// only looking for triangles
		if (scene->mMeshes[m]->mPrimitiveTypes == aiPrimitiveType_TRIANGLE) {
			// create the Mesh
			MeshPtr meshPtr(new Mesh());

			// allocate vertex buffer
			unique_ptr<Vertex_PN[]> verts(new Vertex_PN[scene->mMeshes[m]->mNumVertices]);

			// for each vertex
			for (uint32_t v = 0; v < scene->mMeshes[m]->mNumVertices; ++v) {
				// copy the vertex
				verts[v].pos.assign(scene->mMeshes[m]->mVertices[v].x,
					scene->mMeshes[m]->mVertices[v].y,
					scene->mMeshes[m]->mVertices[v].z);

				verts[v].norm.assign(scene->mMeshes[m]->mNormals[v].x,
					scene->mMeshes[m]->mNormals[v].y,
					scene->mMeshes[m]->mNormals[v].z);
			}

			// allocate index buffer
			int numIndices = scene->mMeshes[m]->mNumFaces * 3;
			unique_ptr<int[]> indices(new int[numIndices]);

			// for each face
			for (uint32_t f = 0; f < scene->mMeshes[m]->mNumFaces; ++f) {
				_ASSERTE(scene->mMeshes[m]->mFaces[f].mNumIndices == 3 && "The face isn't 3 indices!");
				// copy the face indices
				indices[f * 3] = scene->mMeshes[m]->mFaces[f].mIndices[0];
				indices[f * 3 + 1] = scene->mMeshes[m]->mFaces[f].mIndices[1];
				indices[f * 3 + 2] = scene->mMeshes[m]->mFaces[f].mIndices[2];
			}

			// create the RenderBuffers to be passed to the Mesh
			RenderBufferUniquePtr vb(new RenderBufferImpl());
			if (!vb->createFromMemory(RenderBuffer::VertexBuffer,
				verts.get(),
				sizeof(verts.get()),
				sizeof(Vertex_PN), 0))
			{
				debugPrintf("Mesh::importFromFile: failed to create vertex buffer: %s\n");
				return false;
			}

			RenderBufferUniquePtr ib(new RenderBufferImpl());
			if (!ib->createFromMemory(RenderBuffer::IndexBuffer,
				indices.get(),
				sizeof(indices.get()),
				sizeof(int), 0, RenderBuffer::UINT32))
			{
				debugPrintf("Mesh::importFromFile: failed to create index buffer: %s\n");
				return false;
			}

			// build the DrawSet for this mesh (only one needed)
			DrawSet ds;
			ds.startIndex = 0;
			ds.stopIndex = numIndices - 1;
			ds.primitiveCount = scene->mMeshes[m]->mNumFaces;
			ds.type = TriangleList;
			ds.materialIndex = scene->mMeshes[m]->mMaterialIndex;

			// build the RenderEntry list and input material data
			RenderEntry re;
			re.material = m_materials[ds.materialIndex];
			re.meshMaterialId = re.material->getMaterialId();
			re.effectId = re.material->getEffect()->getEffectId();
			re.translucencyType = 0; // assume opaque for now, eventually get from material or effect

			// assemble the Mesh object and add it
			meshPtr->setVertexBuffer(vb);
			meshPtr->setIndexBuffer(ib);
			meshPtr->addDrawSet(ds);
			meshPtr->addRenderEntry(re);
			m_meshes.push_back(meshPtr);
		}
	}

	// get Scene graph
	std::function<void(aiNode*, MeshNode&)> traverseScene;
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

	// everything "assimp" is cleaned up by importer destructor
	return true;
}

#endif