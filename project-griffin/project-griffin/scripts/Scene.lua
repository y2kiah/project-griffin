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

	
	uint64_t griffin_scene_createDataComponentStore(uint64_t scene, uint16_t typeId, uint32_t componentSize, size_t reserve);

	
	void* griffin_scene_getDataComponent(uint64_t scene, uint64_t component);

	
	uint64_t griffin_scene_addDataComponentToEntity(uint64_t scene, uint16_t typeId, uint64_t entity);


	// Scene Node functions

	/**
	* Creates a new entity with a SceneNode component
	* @scene	scene id
	* @movable	true (default) to add a MovementComponent
	* @parentNode	scene node id of the parent scene node, 0 for root node
	* @return	entity id of the newly created scene node
	*/
	
	uint64_t griffin_scene_createNewSceneNode(
				uint64_t scene,
				bool movable,
				uint64_t parentNode);

	/**
	* Creates a new scene entity with SceneNode, ModelInstance and optional animation components
	* @scene	scene id
	* @model	resource id of the model to reference
	* @movable	true (default) to add a MovementComponent
	* @parentNode	scene node id of the parent scene node, 0 for root node
	* @return	entity id of the newly created scene node
	*/
	
	uint64_t griffin_scene_createNewModelInstance(
				uint64_t scene,
				uint64_t model,
				bool movable,
				uint64_t parentNode);

	
	uint64_t griffin_scene_createNewCamera(
				uint64_t scene,
				griffin_CameraParameters* cameraParams,
				const char *name,
				bool shakable,
				uint64_t parentNode);

	
	uint64_t griffin_scene_createLight(uint64_t scene, uint64_t parentEntity);


	// Position, Orientation, Translation, Rotation functions

	//
	//void griffin_scene_setRelativePosition(uint64_t scene, uint64_t entity, griffin_dvec3* pos);

	//
	//griffin_dvec3* griffin_scene_translate(uint64_t scene, uint64_t entity, griffin_dvec3* translation);


]]
local C = ffi.C
