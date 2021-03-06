#include "../Cube.h"
#include <cstdint>
#include <glm/vec3.hpp>

using namespace griffin;
using namespace griffin::render;


struct vertex_pn {
	glm::vec3 p;
	glm::vec3 n;
};


const vertex_pn g_cubeVertexBufferData[] = {
	// front face
	{ { -1.0f,  1.0f, -1.0f }, {  0.0f,  0.0f, -1.0f } }, // 0
	{ { -1.0f, -1.0f, -1.0f }, {  0.0f,  0.0f, -1.0f } },
	{ {  1.0f,  1.0f, -1.0f }, {  0.0f,  0.0f, -1.0f } },
	{ {  1.0f, -1.0f, -1.0f }, {  0.0f,  0.0f, -1.0f } },

	// right face
	{ {  1.0f,  1.0f, -1.0f }, {  1.0f,  0.0f,  0.0f } }, // 4
	{ {  1.0f, -1.0f, -1.0f }, {  1.0f,  0.0f,  0.0f } },
	{ {  1.0f,  1.0f,  1.0f }, {  1.0f,  0.0f,  0.0f } },
	{ {  1.0f, -1.0f,  1.0f }, {  1.0f,  0.0f,  0.0f } },

	// back face
	{ {  1.0f,  1.0f,  1.0f }, {  0.0f,  0.0f,  1.0f } }, // 8
	{ {  1.0f, -1.0f,  1.0f }, {  0.0f,  0.0f,  1.0f } },
	{ { -1.0f,  1.0f,  1.0f }, {  0.0f,  0.0f,  1.0f } },
	{ { -1.0f, -1.0f,  1.0f }, {  0.0f,  0.0f,  1.0f } },

	// left face
	{ { -1.0f,  1.0f,  1.0f }, { -1.0f,  0.0f,  0.0f } }, // 12
	{ { -1.0f, -1.0f,  1.0f }, { -1.0f,  0.0f,  0.0f } },
	{ { -1.0f,  1.0f, -1.0f }, { -1.0f,  0.0f,  0.0f } },
	{ { -1.0f, -1.0f, -1.0f }, { -1.0f,  0.0f,  0.0f } },

	// top face
	{ { -1.0f,  1.0f,  1.0f }, {  0.0f,  1.0f,  0.0f } }, // 16
	{ { -1.0f,  1.0f, -1.0f }, {  0.0f,  1.0f,  0.0f } },
	{ {  1.0f,  1.0f,  1.0f }, {  0.0f,  1.0f,  0.0f } },
	{ {  1.0f,  1.0f, -1.0f }, {  0.0f,  1.0f,  0.0f } },

	// bottom face
	{ {  1.0f, -1.0f, -1.0f }, {  0.0f, -1.0f,  0.0f } }, // 20
	{ {  1.0f, -1.0f,  1.0f }, {  0.0f, -1.0f,  0.0f } },
	{ { -1.0f, -1.0f, -1.0f }, {  0.0f, -1.0f,  0.0f } },
	{ { -1.0f, -1.0f,  1.0f }, {  0.0f, -1.0f,  0.0f } }
};
/*
	  4-----7
	 /|    /|
	0-+---3 |
	| 5---+-6
	|/    |/
	1-----2
*/

const uint8_t g_cubeIndexBufferData[] = {
	// front face
	0, 1, 2, 2, 1, 3,
	// right face
	4, 5, 6, 6, 5, 7,
	// back face
	8, 9, 10, 10, 9, 11,
	// left face
	12, 13, 14, 14, 13, 15,
	// top face
	16, 17, 18, 18, 17, 19,
	// bottom face
	20, 21, 22, 22, 21, 23
};

Cube::Cube() {
	
}
