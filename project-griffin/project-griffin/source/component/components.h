#pragma once
#ifndef _COMPONENTS_H
#define _COMPONENTS_H

#include <vector>
#include <bitset>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Component.h"
#include "ComponentStore.h"

COMPONENT_LIST(
	(Position)
	(Orientation)
	(Person)
)

typedef std::bitset<ComponentTypeCount> ComponentMask;

COMPONENT(Position,
	(glm::vec3, (position, (FieldType::vec3_T, ("3D position coordinate", NIL))))
)

COMPONENT(Orientation,
	(glm::quat, (orientation, (FieldType::quat_T, ("Orientation quaternion", NIL))))
)

COMPONENT(Person,
	(int, (age, (FieldType::int_T, ("The description here", NIL)))),
	(float, (speed, (FieldType::float_T, ("The description 2 here", NIL)))),
	(std::vector<int>, (stuff, (FieldType::vector_T, ("The description 3 here", NIL))))
)

#endif