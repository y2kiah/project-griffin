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
		* SceneNode tracks the transform relative to a parent node and contains ids forming an
		* intrusive hierarchical tree. The SceneGraph is traversed starting at the root to get the
		* worldspace position of each node.
		*/
		COMPONENT(SceneNode,
			(uint32_t,		numChildren,,		"number of children contained"),

			// Flags
			(uint8_t,		positionDirty,,		"position needs recalc"),
			(uint8_t,		orientationDirty,,	"orientation needs recalc"),
			(uint8_t,		_padding_0,[2],		""),

			// transform vars
			(glm::dvec3,	translationLocal,,	"translation relative to parent"),
			(glm::quat,		rotationLocal,,		"local rotation quaternion relative to parent"),

			(glm::dvec3,	positionWorld,,		"position in world space"),
			(glm::quat,		orientationWorld,,	"orientation in world space"),

			// support scale??

			// intrusive tree vars
			(SceneNodeId,	firstChild,,		"first child node index"),
			(SceneNodeId,	nextSibling,,		"next sibling node index"),
			(SceneNodeId,	prevSibling,,		"previous sibling node index"),
			(SceneNodeId,	parent,,			"parent node index")
		)

		/**
		* ModelInstance is a component that goes along with the SceneNode to make an
		* entity represent a unique instance of a model object in the scene.
		*/
		COMPONENT(ModelInstance,
			(Id_T,			modelId,,			"resource id of the model")
		)

		/**
		* The CameraInstance is a component that goes along with the SceneNode to make an
		* entity represent a camera in the scene. The cameraId is obtained from the scene by
		* calling createCamera.
		*/
		COMPONENT(CameraInstance,
			(uint32_t,		cameraId,,			"id of the referenced camera"),
			(char,			name,[32],			"name of the camera")
		)

		/**
		* The LightInstance component that goes along with the SceneNode to make a light in the
		* scene. A CameraInstance component is also created for the entity when the shadowCaster
		* flag is set. If positionViewspace.w is 1.0, it is a position so the light is treated as
		* a point light or spot light. If positionViewspace.w is 0.0, it is a directional light at
		* infinite distance, and the position represents to direction from the scene to the light
		* (in viewspace coordinates). The spot direction is set in worldspace coordinates from the
		* light to the scene for directional lights and spot lights. The distinction between point
		* light and spot light is made with the isSpotLight flag. 
		*/
		COMPONENT(LightInstance,
			(glm::vec4,		positionViewspace,,		"position of light in viewSpace"),
			(glm::vec3,		directionViewspace,,	"direction spotlight is pointing"),
			(glm::vec3,		ambient,,				"ambient light color"),
			(glm::vec3,		diffuseSpecular,,		"diffuse/specular light color"),
			(float,			attenuationConstant,,	"constant added to denominator, usually 1.0"),
			(float,			linearAttenuation,,		"constant multiplied by inverse distance"),
			(float,			quadraticAttenuation,,	"constant multiplied by inverse square of distance"),
			(float,			spotAngleCutoff,,		"spotlight angle cutoff, dot product comparison (% from 90deg)"),
			(float,			spotEdgeBlendPct,,		"spotlight edge blend, in % of spot radius"),
			(float,			volumeRadius,,			"radius from light origin of containing geometry"),
			(uint8_t,		isSpotLight,,			"1 if spot light"),
			(uint8_t,		isPointLight,,			"1 if point light"),
			(uint8_t,		isDirectionalLight,,	"1 if directional light"),
			(uint8_t,		isShadowCaster,,		"1 if light casts shadow")
		)

		/**
		* The MovementComponent is present in all SceneNodes that aren't static. This structure
		* contains prev/next values so the render loop can interpolate between them to get the
		* final rendered position, which is then set in the SceneNode. IT IS UP TO YOU to use this
		* component correctly for movement, in other words you have to set the prev/next values and
		* dirty flags on each update tick so the movement system behaves correctly. It is always an
		* option to NOT use this component to achieve movement in special cases, but you would have
		* to handle the interpolation yourself in a renderTick handler and set the SceneNode values
		* directly.
		*/
		COMPONENT(MovementComponent,
			// flags
			(uint8_t,		translationDirty,,	"position needs recalc"),
			(uint8_t,		rotationDirty,,		"orientation needs recalc"),
			(uint8_t,		_padding_0,[6],		""),

			// movement vars
			(glm::dvec3,	prevTranslation,,	"previous local translation"),
			(glm::dvec3,	nextTranslation,,	"next local translation"),
			(glm::quat,		prevRotation,,		"previous local rotation"),
			(glm::quat,		nextRotation,,		"next local rotation")
		)
		
		/**
		* All entities that can be rendered have a RenderCullInfo component. This stores the data
		* needed to keep track of its indexing in the space partitioning structure, and the data
		* needed to perform frustum culling.
		*/
		COMPONENT(RenderCullInfo,
			(uint32_t,		visibleFrustumBits,,	"bits representing visibility in frustums"),
			(uint32_t,		minWorldAABB,[3],		"AABB integer lower coords in worldspace"),
			(uint32_t,		maxWorldAABB,[3],		"AABB integer upper coords in worldspace"),
			(float,			viewspaceBSphere,[4],	"bounding sphere x,y,z,r in viewspace")
		)

		/**
		*
		*/
		class SceneGraph {
		public:
			explicit SceneGraph(EntityManager& _entityMgr);
			~SceneGraph();

			/**
			* Traverse the scene graph starting at root node and calculate new world positions in
			* breadth-first order. Progress down a branch only when a dirty flag is set.
			*/
			void updateNodeTransforms();

			/**
			* Adds a SceneNode component to the entity and incorporate into the scene graph as a
			* child of the parentNode. If component already exists in entity, the node is moved to
			* the new parent and position.
			* @param entityId	entity to which a SceneNode Component is added
			* @param translationLocal	local position relative to the parent node
			* @param rotationLocal	local rotation relative to the parent node
			* @param parentNodeId	SceneNode component of parent node, NullId_T for the root node
			* @return	ComponentId of the SceneNode added to the entity
			*/
			SceneNodeId addToScene(EntityId entityId, const glm::dvec3& translationLocal,
								   const glm::quat& rotationLocal, SceneNodeId parentNodeId);

			/**
			* See documentation for addToScene. This function takes an EntityId for the parent node
			* rather than a SceneNodeId (component id). Otherwise they are equivalent.
			* @param parentNodeEntityId	EntityId of parent node, NullId_T for the root node.
			* @return	ComponentId of the SceneNode added to the entity, NullId_T if the add fails
			*	or if the parent entity is not part of the scene
			*/
			SceneNodeId addToSceneEntity(EntityId entityId, const glm::dvec3& translationLocal,
										 const glm::quat& rotationLocal, EntityId parentNodeEntityId);

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
			* Starts at sceneNodeId and push all nodes in its ancestor tree into outAncestors. The
			* caller is responsible for the vector outAncestors, it is not cleared before pushing
			* the nodes.
			* @param sceneNodeId	the parent node of the ancestors to collect
			* @param outAncestors	vector to push_back the ancestor SceneNodeIds into
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

			SceneId						m_sceneId = NullId_T;	//<! id of scene this graph belongs to
			SceneNode					m_rootNode;				//<! root of the scene graph, always start traversal from here

			vector_queue<BFSQueueItem>	m_bfsQueue;				//<! queue for breadth-first-search traversal of scene graph
			vector<Id_T>				m_handleBuffer;			//<! buffer used to collect SceneNodeIds and EntityIds in various member functions

			EntityManager&				entityMgr;				//<! reference to the entity manager, set in the constructor
		};

	}
}

#endif