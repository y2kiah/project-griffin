local ffi = require("ffi")

ffi.cdef[[


	typedef struct {
		double x, y, z;
	} griffin_dvec3;
	
	typedef struct {
		float x, y, z, w;
	} griffin_quat;

	enum {
		CAMERA_PERSPECTIVE	= 0,
		CAMERA_ORTHO		= 1
	};

	/**
	* Layout equivalent to CameraParameters
	*/
	typedef struct {
		float			nearClipPlane;
		float			farClipPlane;
		uint32_t		viewportWidth;		//<! 0 to automatically use the main window viewport width
		uint32_t		viewportHeight;		//<! 0 to automatically use the main window viewport height
		float			verticalFieldOfViewDegrees;
		uint8_t			cameraType;			//<! one of the CAMERA_ enum values
		uint8_t			_padding_end[3];
	} griffin_CameraParameters;

	// Functions

	
	uint64_t griffin_scene_createScene(const char name[32], bool makeActive);

	// Entity/Component functions

	
	uint64_t griffin_scene_createComponentStore(uint64_t scene, uint16_t typeId, uint32_t componentSize, size_t reserve);

	
	void* griffin_scene_getComponentData(uint64_t scene, uint64_t component);

	
	uint64_t griffin_scene_addComponentToEntity(uint64_t scene, uint16_t typeId, uint64_t entity);


	// Scene Node functions

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

	//
	//uint64_t griffin_scene_makeMovable(uint64_t scene, uint64_t entity);


	// Position, Orientation, Translation, Rotation functions

	
	void griffin_scene_setRelativePosition(uint64_t scene, uint64_t entity, griffin_dvec3* pos);

	
	griffin_dvec3* griffin_scene_translate(uint64_t scene, uint64_t entity, griffin_dvec3* translation);


]]
local C = ffi.C
