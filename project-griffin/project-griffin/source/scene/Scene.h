#ifndef GRIFFIN_SCENE_H_
#define GRIFFIN_SCENE_H_

#include <cstdint>
#include <string>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <entity/ComponentStore.h>
#include <entity/components.h>
#include <utility/container/vector_queue.h>

namespace griffin {
	namespace scene {

		using namespace griffin::entity;

		COMPONENT(SceneNode,
			(uint8_t,		positionDirty,		"position needs recalc"),
			(uint8_t,		orientationDirty,	"orientation needs recalc"),

			// transform vars
			(glm::dvec3,	translationLocal,	"translation relative to parent"),
			(glm::quat,		orientationLocal,	"local orientation quaternion relative to parent"),

			(glm::dvec3,	positionWorld,		"double-precision position in world space"),
			(glm::quat,		orientationWorld,	"orientation in world space"),

			// support scale??

			// intrusive tree vars
			(uint32_t,		numChildren,		"number of children contained"),
			(ComponentId,	firstChild,			"first child node index"),
			(ComponentId,	nextSibling,		"next sibling node index"),
			(ComponentId,	prevSibling,		"previous sibling node index"),
			(ComponentId,	parent,				"parent node index")
		)

		// TODO: this should be an entity component
		// TODO: optimize memory layout for cache
		/*struct SceneNode {
			uint8_t		positionDirty = 0;
			uint8_t		orientationDirty = 0;

			// transform vars
			glm::dvec3	translationLocal;	//<! translation relative to parent
			glm::quat	orientationLocal;	//<! local orientation quaternion relative to parent
			
			glm::dvec3	positionWorld;		//<! double-precision position in world space
			glm::quat	orientationWorld;	//<! orientation in world space
			
			// support scale??

			// intrusive linked list vars
			uint32_t	numChildren = 0;
			ComponentId	firstChild = NullId_T;	//<! index into array of nodes
			ComponentId	nextSibling = NullId_T;
			ComponentId	prevSibling = NullId_T;
			ComponentId	parent = NullId_T;
		};*/


		// TODO: this should be a system using entity components
		class SceneGraph {
		public:
			explicit SceneGraph();
			~SceneGraph();

			void update();

			// TODO: should this really take a full scene node? We really only want the transform and entity
			ComponentId addNode(SceneNode&& node, ComponentId parentNodeId, EntityId entityId);

			void removeNode(ComponentId nodeId);

		private:
			struct BFSQueueItem {
				uint32_t	nodeIndex;
				uint8_t		ancestorPositionDirty;
				uint8_t		ancestorOrientationDirty;
				glm::dvec3	translationToWorld;
				glm::quat	rotationToWorld;
			};

			ComponentStore<SceneNode>	m_nodes;	//<! contains all scene graph nodes
			ComponentId					m_rootNode;	//<! root of the scene graph, always start traversal from here

			vector_queue<BFSQueueItem>	m_bfsQueue;	//<! queue for breadth-first-search traversal of scene graph
		};

	}
}

#endif