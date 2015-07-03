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
			SceneNode,
			Person
		)

		COMPONENT(Person,
			(int,				age,,		"Person's age in years"),
			(float,				speed,,		"How fast person walks in ft/s"),
			(char,				name,[32],	"Person's name")//,
			//(std::vector<int>,	stuff,	"Person's integer stuff")
		)

	}
}

#endif