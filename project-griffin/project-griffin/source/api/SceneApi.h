/**
* @file SceneApi.h
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_SCENE_API_H_
#define GRIFFIN_SCENE_API_H_

#include <utility/export.h>
#include <cstdint>

#include <utility/container/handle_map.h>


namespace griffin {
	namespace scene { struct CameraParameters; }

	Id_T createEmptySceneNode(Id_T sceneId, Id_T parentEntityId = NullId_T);

	Id_T createModelInstance(Id_T sceneId, bool movable,
							 Id_T parentEntityId = NullId_T, Id_T entityId = NullId_T);

	Id_T createCamera(Id_T sceneId, scene::CameraParameters& cameraParams, const char name[32],
					  Id_T parentEntityId = NullId_T);
}

#ifdef __cplusplus
extern "C" {
#endif

// begin inclusion in lua FFI declaration
#define ffi
#ifdef ffi

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

	GRIFFIN_EXPORT
	uint64_t griffin_scene_createScene(const char name[32], bool makeActive);

	// Entity/Component functions

	GRIFFIN_EXPORT
	uint64_t griffin_scene_createDataComponentStore(uint64_t scene, uint16_t typeId, uint32_t componentSize, size_t reserve);

	GRIFFIN_EXPORT
	void* griffin_scene_getDataComponent(uint64_t scene, uint64_t component);

	GRIFFIN_EXPORT
	uint64_t griffin_scene_addDataComponentToEntity(uint64_t scene, uint16_t typeId, uint64_t entity);


	// Scene Node functions

	/**
	* Creates a new entity with a SceneNode component
	* @scene	scene id
	* @parentEntity	entity id of the parent scene node, 0 for root node
	* @return	entity id of the newly created scene node
	*/
	GRIFFIN_EXPORT
	uint64_t griffin_scene_createEmptySceneNode(uint64_t scene, uint64_t parentEntity);

	/**
	* Creates a new scene entity with SceneNode, ModelInstance and optional animation components
	* @scene	scene id
	* @parentEntity	entity id of the parent scene node, 0 for root node
	* @model	resource id of the model to reference
	* @return	entity id of the newly created scene node
	*/
	GRIFFIN_EXPORT
	uint64_t griffin_scene_createModelInstance(uint64_t scene, uint64_t parentEntity, uint64_t model);

	GRIFFIN_EXPORT
	uint64_t griffin_scene_createCamera(uint64_t scene, uint64_t parentEntity,
										griffin_CameraParameters* cameraParams, const char name[32]);

	GRIFFIN_EXPORT
	uint64_t griffin_scene_createLight(uint64_t scene, uint64_t parentEntity);

	//GRIFFIN_EXPORT
	//uint64_t griffin_scene_makeMovable(uint64_t scene, uint64_t entity);


	// Position, Orientation, Translation, Rotation functions

	GRIFFIN_EXPORT
	void griffin_scene_setRelativePosition(uint64_t scene, uint64_t entity, griffin_dvec3* pos);

	GRIFFIN_EXPORT
	griffin_dvec3* griffin_scene_translate(uint64_t scene, uint64_t entity, griffin_dvec3* translation);

#endif ffi
#undef ffi
// end inclusion in lua FFI declaration

#ifdef __cplusplus
}
#endif

#endif