/**
* @file SceneGraph.h
* @author Jeff Kiah
*/
#ifndef GRIFFIN_SCENEGRAPH_H_
#define GRIFFIN_SCENEGRAPH_H_

#include <cstdint>
#include <string>
#include <memory>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <entity/ComponentStore.h>
#include <entity/components.h>
#include <utility/container/vector_queue.h>


namespace griffin {
	// Forward Declarations
	namespace entity { class EntityManager; }

	namespace scene {

		using namespace griffin::entity;

		typedef griffin::Id_T    SceneId;
		typedef griffin::Id_T    SceneNodeId;


		// Structs

		/**
		*
		*/
		COMPONENT(SceneNode,
			(uint8_t,		positionDirty,		"position needs recalc"),
			(uint8_t,		orientationDirty,	"orientation needs recalc"),

			// transform vars
			(glm::dvec3,	translationLocal,	"translation relative to parent"),
			(glm::quat,		rotationLocal,		"local rotation quaternion relative to parent"),

			(glm::dvec3,	positionWorld,		"double-precision position in world space"),
			(glm::quat,		orientationWorld,	"orientation in world space"),

			// support scale??

			// intrusive tree vars
			(uint32_t,		numChildren,		"number of children contained"),
			(SceneNodeId,	firstChild,			"first child node index"),
			(SceneNodeId,	nextSibling,		"next sibling node index"),
			(SceneNodeId,	prevSibling,		"previous sibling node index"),
			(SceneNodeId,	parent,				"parent node index")
		)

		
		/**
		*
		*/
		class SceneGraph {
		public:
			explicit SceneGraph(EntityManager& _entityMgr);
			~SceneGraph();

			void update();

			/**
			* Adds a SceneNode component to the entity and incorporate into the scene graph as a
			* child of the parentNode. If component already exists in entity, the node is moved to
			* the new parent and position.
			* @param entityId	entity to which a SceneNode is added
			* @param translationLocal	local position relative to the parent node
			* @param rotationLocal	local rotation relative to the parent node
			* @param parentNodeId	SceneNode component of parent node, NullId_T for the root node
			* @return	ComponentId of the SceneNode added to the entity
			*/
			SceneNodeId addToScene(EntityId entityId, const glm::dvec3& translationLocal,
								   const glm::quat& rotationLocal, SceneNodeId parentNodeId);

			/**
			* Removes the SceneNode component from the entity and also fixes up the scene graph.
			* Use this function when the component id is known.
			* @param sceneNodeId	ComponentId of the SceneNode to remove
			* @param cascade	If true the node's ancestors are given to the node's parent not
			*			destroyed. If false the node's ancestors are removed.
			* @param removedEntities	vector to push the removed ancestor entity ids, not
			*			including the top removed entity, or nullptr if you don't care. The caller
			*			is responsible for the vector, this function only uses push_back.
			* @return	true if removed, false if SceneNode component not present
			*/
			bool removeFromScene(SceneNodeId sceneNodeId, bool cascade = true, std::vector<EntityId>* removedEntities = nullptr);

			/**
			* Removes the SceneNode component from the entity and also fixes up the scene graph.
			* Use this function when only the entity id is known.
			* @param sceneNodeId	ComponentId of the SceneNode to remove
			* @param cascade	If true the node's ancestors are given to the node's parent not
			*			destroyed. If false the node's ancestors are removed.
			* @param removedEntities	vector to push the removed ancestor entity ids, not
			*			including the top removed entity, or nullptr if you don't care. The caller
			*			is responsible for the vector, this function only uses push_back.
			* @return	true if removed, false if SceneNode component not present
			*/
			bool removeEntityFromScene(EntityId entityId, bool cascade = true, std::vector<EntityId>* removedEntities = nullptr);

			/**
			* Moves a sceneNode referenced by sceneNodeId from its current parent to a new parent.
			* Take care not to move the node to a parent deeper in its own branch, as that would
			* break the tree, and the case is not checked for performance reasons. This function
			* does cascade so all ancestors of sceneNodeId move with it.
			* @param sceneNodeId	id of the scene node to move
			* @param moveToParent	the new parent id
			* @return	true if the node is moved, false if a given id is invalid or if
			*		moveToParent is already the current parent
			*/
			bool moveNode(SceneNodeId sceneNodeId, SceneNodeId moveToParent);

			/**
			* Moves the sceneNode referenced by siblingToMove, and each of its siblings by getting
			* the full list from the current parent, and moving each to the front of the new
			* parent's child list. Take care not to move the siblings to a parent deeper in their
			* own branch, as that would break the tree, and the case is not checked for performance
			* reasons. This function does cascade so all ancestors of the nodes move with them.
			* @param siblingToMove	id of the scene node to move, along with all siblings
			* @param moveToParent	the new parent id
			* @return	true if the nodes are moved, false if a given id is invalid or if
			*		moveToParent is already the current parent
			*/
			bool moveAllSiblings(SceneNodeId siblingToMove, SceneNodeId moveToParent);

			/**
			* Iterates through the child linked list and returns the last child (where nextSibling
			* is NullId_T). This is a O(n) linear operation but most lists besides root should be
			* pretty short.
			* @param sceneNodeId	the parent node of the last child to look for
			* @return	scene node id of the last child, or NullId_T if there are no children
			*/
			SceneNodeId getLastImmediateChild(SceneNodeId sceneNodeId) const;

			/**
			*
			*/
			void collectAncestors(SceneNodeId sceneNodeId, std::vector<SceneNodeId>& outAncestors) const;

			/**
			* Sets the sceneId of the scene owning this scene graph, stored in SceneNodes
			*/
			void setSceneId(SceneId sceneId) {
				m_sceneId = sceneId;
			}

		private:
			struct BFSQueueItem {
				SceneNode*	sceneNode;
				uint8_t		ancestorPositionDirty;
				uint8_t		ancestorOrientationDirty;
				glm::dvec3	translationToWorld;
				glm::quat	rotationToWorld;
			};

			SceneId						m_sceneId;	//<! id of scene this graph belongs to
			SceneNode					m_rootNode;	//<! root of the scene graph, always start traversal from here

			vector_queue<BFSQueueItem>	m_bfsQueue;	//<! queue for breadth-first-search traversal of scene graph

			EntityManager&				entityMgr;	//<! reference to the entity manager, set in the constructor
		};

	}
}

#endif