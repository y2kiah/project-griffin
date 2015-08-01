local ffi = require("ffi")

ffi.cdef[[

	//typedef void(*Callback_T)(griffin_FrameMappedInput*);

	// Functions

	
	uint64_t griffin_scene_createScene(const char name[32], bool makeActive);


	// Scene Node and Entity/Component APIs

	/**
	* Creates a new entity with a SceneNode component
	* @scene	scene id
	* @parentEntity	entity id of the parent scene node, 0 for root node
	* @return	entity id of the newly created scene node
	*/
	
	uint64_t griffin_scene_createEmptySceneNode(uint64_t scene, uint64_t parentEntity);

	/**
	* Creates a new entity with SceneNode and MeshInstanceContainer components
	* @scene	scene id
	* @parentEntity	entity id of the parent scene node, 0 for root node
	* @mesh		resource id of the mesh to reference
	* @return	entity id of the newly created scene node
	*/
	
	uint64_t griffin_scene_createMeshInstance(uint64_t scene, uint64_t parentEntity, uint64_t mesh);

	
	uint64_t griffin_scene_createCamera(uint64_t scene, uint64_t parentEntity);

	
	uint64_t griffin_scene_createLight(uint64_t scene, uint64_t parentEntity);



]]
local C = ffi.C
