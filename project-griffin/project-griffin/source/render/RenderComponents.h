/**
* @file RenderComponents.h
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_RENDER_COMPONENTS_H_
#define GRIFFIN_RENDER_COMPONENTS_H_

#include <entity/components.h>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace griffin {
	namespace render {

		/**
		* Tracks time and blend factor of up to 6 animation tracks for a mesh instance. If there
		* are more than 6 animations for a mesh, the entity will have more than one MeshAnimation-
		* Component, with baseAnimationIndex incremented by 6 for each additional component. The
		* prev/next values are interpolated for rendering.
		*/
		COMPONENT(MeshAnimationTrackComponent,
			(uint32_t,		baseAnimationIndex,,			"base animation track index"),
			(float,			prevAnimationTime,[6],			"previous animation time in seconds"),
			(float,			nextAnimationTime,[6],			"next animation time in seconds"),
			(float,			prevAnimationBlend,[6],			"previous blend factor from 0 to 1, 0 for disabled"),
			(float,			nextAnimationBlend,[6],			"next blend factor from 0 to 1, 0 for disabled")
		)

		/**
		* Stores an interpolated and blended node transform per node for a mesh instance. There is
		* one of these components per unique node that is animated on a mesh, for each instance of
		* the mesh. The prev/next values are written in the update loop and interpolated in the
		* render loop.
		*/
		COMPONENT(MeshNodeAnimationComponent,
			(uint32_t,		nodeIndex,,						"MeshSceneNode index"),
			(uint8_t,		translationActive,,				"1 = use this translation, 0 = use default translation"),
			(uint8_t,		rotationActive,,				"1 = use this rotation, 0 = use default rotation"),
			(uint8_t,		scalingActive,,					"1 = use this scaling, 0 = use default scaling"),
			(uint8_t,		_padding_0,,					""),
			(glm::vec3,		prevTranslationLocal,,			"previous interpolated and blended translation relative to parent"),
			(glm::vec3,		nextTranslationLocal,,			"next interpolated and blended translation relative to parent"),
			(glm::quat,		prevRotationLocal,,				"previous interpolated and blended rotation relative to parent"),
			(glm::quat,		nextRotationLocal,,				"next interpolated and blended rotation relative to parent"),
			(glm::vec3,		prevScalingLocal,,				"previous interpolated and blended scaling relative to parent"),
			(glm::vec3,		nextScalingLocal,,				"next interpolated and blended scaling relative to parent")
		)

		/**
		*
		*/
		COMPONENT(MaterialOverrideComponent,
			(float,			todo,,							"")
		)
	}
}

#endif