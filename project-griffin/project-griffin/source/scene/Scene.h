#ifndef GRIFFIN_SCENE_H_
#define GRIFFIN_SCENE_H_

#include <cstdint>
#include <string>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <utility/container/handle_map.h>

namespace griffin {
	namespace scene {

		using std::string;

		struct SceneNode {
			bool		dirty = false;
			glm::dvec3	positionWorld;		//<! double-precision world position
			glm::dmat4	transform;			//<!
			uint32_t	numChildren = 0;
			Id_T		firstChild = {};
			Id_T		parent = {};		//<! index into array of nodes
			string		name;
		};


		class SceneGraph {
			//MeshSceneNode * sceneNodes = nullptr;	//<! array of nodes starting with root, in breadth-first order
			handle_map<SceneNode>	nodes;
		};

	}
}

#endif