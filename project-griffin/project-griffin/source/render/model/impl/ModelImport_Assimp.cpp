#ifdef GRIFFIN_TOOLS_BUILD
#include "../ModelImport_Assimp.h"

#pragma comment( lib, "assimp.lib" )

#include <assimp/Importer.hpp>	// C++ importer interface
#include <assimp/scene.h>		// Output data structure
#include <assimp/postprocess.h>	// Post processing flags
#include <assimp/config.h>		// Configuration properties

#include <string>
#include <memory>
#include <functional>
#include <vector>

// for Mesh
#include <glm/vec3.hpp>

using namespace Assimp;
using std::string;
using std::wstring;
using std::unique_ptr;
using std::vector;

struct Mesh {
	// Constructors / destructor
	explicit Mesh() {}
	Mesh(Mesh&& other)
	{
		numVertices = other.numVertices;
		vertices = other.vertices;
		normals = other.normals;
		indices = other.indices;
		other.numVertices = 0;
		other.vertices = nullptr;
		other.normals = nullptr;
		other.indices = nullptr;
	};
	Mesh(const Mesh&) = delete;
	~Mesh() {
		delete[] vertices;
		delete[] normals;
		delete[] indices;
	}
	
	// Functions
	//void setVertexBuffer(RenderBufferUniquePtr &vb)	{ m_vertexBuffer = std::move(vb); }
	//void setIndexBuffer(RenderBufferUniquePtr &ib)	{ m_indexBuffer = std::move(ib); }
	//void addDrawSet(const DrawSet &ds)				{ m_drawSets.push_back(ds); }
	//void addRenderEntry(const RenderEntry &re)		{ m_renderEntries.push_back(re); }

	// Variables
	uint32_t		numVertices = 0;
	glm::vec3 *		vertices = nullptr;
	// consider interleaving normals, bitangents, colors, etc. in a single array
	// use bitset to flag components that are present in the array
	glm::vec3 *		normals = nullptr;
	uint32_t *		indices = nullptr;
	//vector<DrawSet>			drawSets;		// using index buffers
};


/**
* Imports a model using assimp
*/
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
	// get Meshes
	uint32_t numMeshes = scene->mNumMeshes;
	vector<Mesh> meshes;
	meshes.reserve(numMeshes);
	
	for (uint32_t m = 0; m < numMeshes; ++m) {
		meshes.emplace_back();
		auto& myMesh = meshes[m];
		auto& assimpMesh = *scene->mMeshes[m];

		// only looking for triangle meshes
		if (assimpMesh.mPrimitiveTypes == aiPrimitiveType_TRIANGLE) {
			
			// allocate vertex buffer
			uint32_t numVertices = assimpMesh.mNumVertices;
			myMesh.numVertices = numVertices;
			unique_ptr<glm::vec3[]> verts(new glm::vec3[numVertices]);

			// for each vertex
			for (uint32_t v = 0; v < numVertices; ++v) {
				// copy the vertex
				verts[v].x = assimpMesh.mVertices[v].x;
				verts[v].y = assimpMesh.mVertices[v].y;
				verts[v].z = assimpMesh.mVertices[v].z;
			}

			// allocate normal buffer
			unique_ptr<glm::vec3[]> normals(new glm::vec3[numVertices]);
			if (assimpMesh.mNormals != nullptr) {
				for (uint32_t n = 0; n < numVertices; ++n) {
					normals[n].x = assimpMesh.mNormals[n].x;
					normals[n].y = assimpMesh.mNormals[n].y;
					normals[n].z = assimpMesh.mNormals[n].z;
				}
			}
			else {
				// normals not in mesh data, build the normals ourselves here??
			}

			// allocate index buffer
			uint32_t numFaces = assimpMesh.mNumFaces;
			uint32_t numIndices = numFaces * 3;
			unique_ptr<uint32_t[]> indices(new uint32_t[numIndices]);

			// for each face, copy the face indices
			for (uint32_t f = 0; f < numFaces; ++f) {
				assert(assimpMesh.mFaces[f].mNumIndices == 3 && "the face doesn't have 3 indices");

				indices[f * 3] = assimpMesh.mFaces[f].mIndices[0];
				indices[f * 3 + 1] = assimpMesh.mFaces[f].mIndices[1];
				indices[f * 3 + 2] = assimpMesh.mFaces[f].mIndices[2];
			}

			// create the RenderBuffers to be passed to the Mesh
/*			RenderBufferUniquePtr vb(new RenderBufferImpl());
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
*/

			// build the DrawSet for this mesh (only one needed)
/*			DrawSet ds;
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
*/

			// assemble the Mesh object
			myMesh.vertices = verts.release();
			myMesh.normals = normals.release();
			myMesh.indices = indices.release();
		}
	}

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

#endif