/**
* @file Scene.h
* @author Jeff Kiah
*/
#ifndef GRIFFIN_SCENE_H_
#define GRIFFIN_SCENE_H_

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
		using std::weak_ptr;

		typedef griffin::Id_T    SceneId;

		// Variables
		extern weak_ptr<entity::EntityManager> g_entityManager;

		// Structs

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
			(ComponentId,	firstChild,			"first child node index"),
			(ComponentId,	nextSibling,		"next sibling node index"),
			(ComponentId,	prevSibling,		"previous sibling node index"),
			(ComponentId,	parent,				"parent node index"),

			(SceneId,		scene,				"id of the scene this belongs to")
		)

		
		/**
		*
		*/
		class SceneGraph {
		public:
			explicit SceneGraph();
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
			ComponentId addToScene(EntityId entityId, const glm::dvec3& translationLocal,
								   const glm::quat& rotationLocal, ComponentId parentNodeId);

			/**
			* Removes the SceneNode component from the entity and also fixes up the scene graph. If
			* the node has children, they are given to the node's parent and not destroyed. This is
			* therefor a non-cascading removal effecting only one node.
			* @param entityId	entity from which a SceneNode is removed
			* @return	true if removed, false if SceneNode component not present
			*/
			bool removeFromScene(EntityId entityId);

			/**
			* Removes the SceneNode component from the entity and also fixes up the scene graph. If
			* the node has ancestors, they are also removed, this is a cascading removal.
			* @param entityId	entity from which a SceneNode is removed
			* @return	the set of entities where SceneNodes were removed (entity + ancestors)
			*/
			std::vector<EntityId> removeTreeFromScene(EntityId entityId);

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
		};


		/**
		*
		*/
		struct Scene {
			SceneGraph	sceneGraph;
			// contains Cameras???
			// contains layer id for RenderEntry
			std::string	name;
		};


		/**
		*
		*/
		class SceneManager {
		public:
			explicit SceneManager();
			~SceneManager();

			Scene& getScene(SceneId sceneId) { return m_scenes[sceneId]; }
			
			SceneId createScene(const std::string& name) {
				auto sceneId = m_scenes.insert({ SceneGraph(), name });
				m_scenes[sceneId].sceneGraph.setSceneId(sceneId);
				return sceneId;
			}

		private:
			handle_map<Scene> m_scenes;
		};


		typedef std::shared_ptr<SceneManager> SceneManagerPtr;
	}
}

#endif