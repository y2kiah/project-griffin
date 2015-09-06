/**
* @file RenderComponents.h
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_RENDER_COMPONENTS_H_
#define GRIFFIN_RENDER_COMPONENTS_H_

#include <entity/components.h>

namespace griffin {
	namespace render {

		/**
		* Tracks the time and blend factor of up to 6 mesh animations for a mesh instance. If there
		* are more than 6 animations for a mesh, the nextAnimationId forms an embedded singly-
		* linked list of components, and the subsequent component's base index is incremented by 6.
		*/
		COMPONENT(MeshAnimationComponent,
			(float,			animationTime,[6],	"animation time in seconds"),
			(float,			animationBlend,[6],	"blend factor from 0 to 1, 0 for disabled"),
			(Id_T,			nextAnimationId,,	"id of next mesh animation component"),
			(uint32_t,		baseAnimationIndex,,"base animation index tracked by this component"),
			
			(uint32_t,		_padding_end,,		"")
		)

		/**
		*
		*/
		COMPONENT(MaterialOverrideComponent,
			(float,			todo,,				"")
		)
	}
}

#endif