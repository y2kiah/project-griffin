#pragma once
#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <vector>
#include <bitset>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Component.h"
#include "ComponentStore.h"

// disable BOOST_PP_EXPAND_I warning that showed up in 1.57
// see this thread http://comments.gmane.org/gmane.comp.lib.boost.user/82952
#pragma warning(disable:4003)

// This can be improved. Instead of defining a list and each component separately, define just the
// components within a macro, and import this file in multiple locations, each time redefining the
// surrounding macro, thus interpreting the data in different ways. So I can define one list of
// components and get the list and struct definitions out of it, in the correct order.

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
	(int, (age, (FieldType::int_T, ("Person's age in years", NIL)))),
	(float, (speed, (FieldType::float_T, ("How fast person walks in ft/s", NIL)))),
	(std::string, (name, (FieldType::string_T, ("Person's name", NIL)))),
	(std::vector<int>, (stuff, (FieldType::vectorInt_T, ("Person's integer stuff", NIL))))
)

#endif