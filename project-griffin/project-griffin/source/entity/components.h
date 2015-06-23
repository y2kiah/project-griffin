#pragma once
#ifndef GRIFFIN_COMPONENTS_H_
#define GRIFFIN_COMPONENTS_H_

#include <vector>
#include <bitset>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Component.h"
#include "ComponentStore.h"

namespace griffin {
	namespace entity {

		/**
		* The component list forward declares all component structs, defines an enum to give each type
		* known at compile type a unique id, and also defines a type cast to get the component's type from
		* its id. This does not actually define the component, that's done using the COMPONENT macro.
		*/
		COMPONENT_LIST(
			(SceneNode)
			(Orientation)
			(Transform)
			(Person)
		)

		/**
		* MAX_COMPONENTS includes compile-time components plus script-based components
		*/
		#define MAX_COMPONENTS	64
		typedef std::bitset<MAX_COMPONENTS> ComponentMask;

		COMPONENT(SceneNode,
			(glm::dvec3,		(position,		(FieldType::dvec3_T,		("3D position coordinate", NIL))))
		)

		COMPONENT(Transform,
			(glm::dvec3,		(position,		(FieldType::dvec3_T,		("3D position coordinate", NIL)))),
			(glm::quat,			(orientation,	(FieldType::quat_T,			("Orientation quaternion", NIL))))
		)

		COMPONENT(Person,
			(int,				(age,			(FieldType::int_T,			("Person's age in years", NIL)))),
			(float,				(speed,			(FieldType::float_T,		("How fast person walks in ft/s", NIL)))),
			(std::string,		(name,			(FieldType::string_T,		("Person's name", NIL)))),
			(std::vector<int>,	(stuff,			(FieldType::vectorInt_T,	("Person's integer stuff", NIL))))
		)

	}
}

#endif