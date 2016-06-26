#pragma once
#ifndef GRIFFIN_SHADER_PROGRAM_LAYOUTS_H_
#define GRIFFIN_SHADER_PROGRAM_LAYOUTS_H_

#include <cstdint>
#include <utility/enum.h>
#include <glm/mat4x4.hpp>

namespace griffin {
	namespace render {
		// Enums

		/**
		* Enum of all standard uniform blocks. These are bound by name using an enum-to-string
		* macro, so exact name matching is important. The value corresponds to the binding point
		* index, with +1 added to the value.
		*/
		MakeEnum(UBOType, uint8_t,
				 (CameraUniforms)
				 (ObjectUniforms)
				 ,);

		/**
		* Enum of the standard vertex buffer layouts.
		*/
		enum VertexLayoutLocation : uint8_t {
			VertexLayout_Position      = 0,
			VertexLayout_Normal        = 1,
			VertexLayout_Tangent       = 2,
			VertexLayout_Bitangent     = 3,
			VertexLayout_TextureCoords = 4,  // consumes up to 8 locations
			VertexLayout_Colors        = 12  // consumes up to 8 locations
		};


		// Structs

		/**
		* GLSL layout(std140)
		*/
		struct CameraUniformsUBO {
			glm::mat4	projection;
			glm::mat4	viewProjection;
			float		frustumNear;
			float		frustumFar;
			float		inverseFrustumDistance;
		};

		/**
		* GLSL layout(std140)
		*/
		struct ObjectUniformsUBO {
			glm::mat4	modelToWorld;
			glm::mat4	modelView;
			glm::mat4	modelViewProjection;
			glm::mat4	normalMatrix;
		};
	}
}

#endif