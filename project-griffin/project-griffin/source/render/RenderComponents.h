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
#include <bitset>

using std::bitset;

namespace griffin {
	namespace render {

		#define MAX_MESH_ANIMATION_TRACKS	32 // may need 64 or more depending on actual art assets

		struct AnimationTrackInstance {
			float	prevAnimationTime;		//<! previous animation time in seconds
			float	nextAnimationTime;		//<! next animation time in seconds
			float	prevAnimationWeight;	//<! previous blend factor from 0 to 1, 0 for disabled, 1 for fully enabled
			float	nextAnimationWeight;	//<! next blend factor from 0 to 1, 0 for disabled, 1 for fully enabled
		};

		/**
		* Tracks time and blend factor of all animation tracks for a mesh instance. The prev/next
		* values are interpolated for rendering.
		*/
		COMPONENT(MeshAnimationTrack,
			(bitset<MAX_MESH_ANIMATION_TRACKS>, playing,,					"bitset indicating animation is currently playing"),
			(bitset<MAX_MESH_ANIMATION_TRACKS>, looped,,					"bitset indicating animation loops when played"),
			(int8_t,					queuedAnimations,[8],				"queued animation indices"),
			(AnimationTrackInstance,	animation,[MAX_MESH_ANIMATION_TRACKS], "animation track instance data")
		)

		/**
		* Stores an interpolated and blended node transform per node for a mesh instance. There is
		* one of these components per unique node that is animated on a mesh, for each instance of
		* the mesh. The prev/next values are written in the update loop and interpolated in the
		* render loop.
		*/
		COMPONENT(MeshNodeAnimation,
			(uint32_t,		nodeIndex,,				"MeshSceneNode index"),
			(uint8_t,		prevActive,,			"1 = use this previous transform, 0 = use default transform"),
			(uint8_t,		nextActive,,			"1 = use this next transform, 0 = use default transform"),
			(uint8_t,		_padding_0,[2],			""),
			(glm::vec3,		prevTranslationLocal,,	"previous interpolated and blended translation relative to parent"),
			(glm::vec3,		nextTranslationLocal,,	"next interpolated and blended translation relative to parent"),
			(glm::quat,		prevRotationLocal,,		"previous interpolated and blended rotation relative to parent"),
			(glm::quat,		nextRotationLocal,,		"next interpolated and blended rotation relative to parent"),
			(glm::vec3,		prevScalingLocal,,		"previous interpolated and blended scaling relative to parent"),
			(glm::vec3,		nextScalingLocal,,		"next interpolated and blended scaling relative to parent"),
			(glm::vec3,		defaultTranslation,,	"default node translation"),
			(glm::quat,		defaultRotation,,		"default node rotation"),
			(glm::vec3,		defaultScaling,,		"default node scaling")
		)

		/**
		*
		*/
		COMPONENT(MaterialOverride,
			(float,			todo,,							"")
		)
	}
}

#endif