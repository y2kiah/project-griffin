/**
* @file SceneApi.h
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_SCENE_API_H_
#define GRIFFIN_SCENE_API_H_

#include <utility/export.h>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// begin inclusion in lua FFI declaration
#define ffi
#ifdef ffi
	//typedef void(*Callback_T)(griffin_FrameMappedInput*);

	// Functions

	GRIFFIN_EXPORT
	uint64_t griffin_scene_createScene(const char name[32], bool makeActive);


	// Scene Node and Entity/Component APIs

	/**
	* Creates a new entity with a SceneNode component
	* @scene	scene id
	* @parentEntity	entity id of the parent scene node, 0 for root node
	* @return	entity id of the newly created scene node
	*/
	GRIFFIN_EXPORT
	uint64_t griffin_scene_createEmptySceneNode(uint64_t scene, uint64_t parentEntity);

	/**
	* Creates a new entity with SceneNode and MeshInstanceContainer components
	* @scene	scene id
	* @parentEntity	entity id of the parent scene node, 0 for root node
	* @mesh		resource id of the mesh to reference
	* @return	entity id of the newly created scene node
	*/
	GRIFFIN_EXPORT
	uint64_t griffin_scene_createMeshInstance(uint64_t scene, uint64_t parentEntity, uint64_t mesh);

	GRIFFIN_EXPORT
	uint64_t griffin_scene_createCamera(uint64_t scene, uint64_t parentEntity);

	GRIFFIN_EXPORT
	uint64_t griffin_scene_createLight(uint64_t scene, uint64_t parentEntity);


#endif ffi
#undef ffi
// end inclusion in lua FFI declaration

#ifdef __cplusplus
}
#endif

#endif