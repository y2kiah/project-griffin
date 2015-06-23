#ifndef GRIFFIN_SCENE_H_
#define GRIFFIN_SCENE_H_

#include <cstdint>
#include <string>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <utility/container/handle_map.h>
#include <utility/container/vector_queue.h>

namespace griffin {
	namespace scene {

		// TODO: this should be an entity component
		// TODO: optimize memory layout for cache
		struct SceneNode {
			uint8_t		positionDirty = 0;
			uint8_t		orientationDirty = 0;

			// transform vars
			glm::dvec3	translationLocal;	//<! translation relative to parent
			glm::quat	orientationLocal;	//<! local orientation quaternion relative to parent
			
			glm::dvec3	positionWorld;		//<! double-precision position in world space
			glm::quat	orientationWorld;	//<! orientation in world space
			
			// support scale??

			// should contain entity handle

			// intrusive linked list vars
			uint32_t	numChildren = 0;
			Id_T		firstChild = NullId_T;	//<! index into array of nodes
			Id_T		nextSibling = NullId_T;
			Id_T		prevSibling = NullId_T;
			Id_T		parent = NullId_T;
		};


		// TODO: this should be a system using entity components
		class SceneGraph {
		public:
			explicit SceneGraph();
			~SceneGraph();

			void update();

			// TODO: should this really take a full scene node? We really only want the transform and entity
			Id_T addNode(SceneNode&& node, Id_T parentId = NullId_T);

			void removeNode(Id_T nodeId);

		private:
			struct BFSQueueItem {
				uint32_t	nodeIndex;
				uint8_t		ancestorPositionDirty;
				uint8_t		ancestorOrientationDirty;
				glm::dvec3	translationToWorld;
				glm::quat	rotationToWorld;
			};

			handle_map<SceneNode>		m_nodes;	//<! contains all scene graph nodes
			Id_T						m_rootNode;	//<! root of the scene graph, always start traversal from here

			vector_queue<BFSQueueItem>	m_bfsQueue;	//<! queue for breadth-first-search traversal of scene graph
		};

	}
}

#endif