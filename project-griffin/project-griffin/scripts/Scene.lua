local ffi = require("ffi")

ffi.cdef[[


	enum {
		CAMERA_PERSPECTIVE	= 0,
		CAMERA_ORTHO		= 1
	};

	/**
	* Layout equivalent to CameraParameters
	*/
	typedef struct {
		float		nearClipPlane;
		float		farClipPlane;
		uint32_t	viewportWidth;		//<! 0 to automatically use the main window viewport width
		uint32_t	viewportHeight;		//<! 0 to automatically use the main window viewport height
		float		verticalFieldOfViewDegrees;
		uint8_t		cameraType;			//<! one of the CAMERA_ enum values
		uint8_t		_padding_end[3];
	} griffin_CameraParameters;

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

	
	uint64_t griffin_scene_createCamera(uint64_t scene, uint64_t parentEntity,
										griffin_CameraParameters* cameraParams, const char name[32]);

	
	uint64_t griffin_scene_createLight(uint64_t scene, uint64_t parentEntity);



]]
local C = ffi.C
