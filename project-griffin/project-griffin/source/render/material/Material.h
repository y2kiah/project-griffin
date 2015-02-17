#pragma once
#ifndef GRIFFIN_MATERIAL_H_
#define GRIFFIN_MATERIAL_H_

#include <glm/vec3.hpp>

namespace griffin {
	namespace render {
		/*
		enum MaterialFlags : uint16_t {
			Material_None        = 0,
			Material_Ambient     = 1,
			Material_Diffuse     = 1 << 1,
			Material_Specular    = 1 << 2,
			Material_Emissive    = 1 << 3,
			Material_Reflective  = 1 << 4,
			Material_Transparent = 1 << 5
		};

		struct Material_GL {
			VertexFlags   vertexRequirementFlags;
			MaterialFlags materialFlags;

		};
		*/

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