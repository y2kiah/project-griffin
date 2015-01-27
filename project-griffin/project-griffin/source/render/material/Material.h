#pragma once
#ifndef GRIFFIN_MATERIAL_H_
#define GRIFFIN_MATERIAL_H_

#include <glm/vec3.hpp>

namespace griffin {
	namespace render {

		struct Material {
			glm::vec3 diffuseColor;
			glm::vec3 ambientColor;
			glm::vec3 specularColor;
			glm::vec3 emissiveColor;
			glm::vec3 reflectiveColor;
			glm::vec3 transparentColor;
			
			//float     diffusePower = 0;
			float     opacity = 1.0f;
			float     reflectivity = 0;
			float     refracti = 0;
			float     shininess = 0;
			float     shininessStrength = 0;
		};

	}
}

#endif