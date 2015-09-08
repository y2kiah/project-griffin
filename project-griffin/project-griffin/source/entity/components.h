/**
* @file components.h
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_COMPONENTS_H_
#define GRIFFIN_COMPONENTS_H_

#include "EntityTypedefs.h"
#include <vector>
#include <bitset>
#include "Component.h"

namespace griffin {
	namespace entity {

		/**
		* MAX_COMPONENTS includes compile-time components plus script-based components
		*/
		#define MAX_COMPONENTS	64
		typedef std::bitset<MAX_COMPONENTS> ComponentMask;


		/**
		* The component list forward declares all component structs, defines an enum to give each type
		* known at compile type a unique id, and also defines a type cast to get the component's type from
		* its id. This does not actually define the component, that's done using the COMPONENT macro.
		*/
		COMPONENT_LIST(
			SceneNode,					//<! System: SceneGraph, all entities that exist at a position in the scene get this
			ModelInstanceContainer,		//<! System: SceneGraph, model instance, submitted to renderer if visible
			CameraInstanceContainer,	//<! System: SceneGraph, camera instance, submitted to renderer if active
			MovementComponent,			//<! System: SceneGraph, all entities that can move in the scene with auto interpolation
			RenderCullInfo,				//<! System: SceneGraph, all entitied that can be rendered to the screen

			MeshAnimationTrackComponent,//<! System: Animation, mesh instance animation times and blends
			MeshNodeAnimationComponent,	//<! System: Animation, mesh instance interpolated and blended node transform
			MaterialOverrideComponent	//<! System: Render, mesh instance material overrides
		)

	}
}

#endif