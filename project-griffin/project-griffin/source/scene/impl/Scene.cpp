#include "../Scene.h"
#include <SDL_log.h>
#include <entity/Entity.h>
#include <utility/memory_reserve.h>

namespace griffin {
	namespace scene {

		// Globals

		weak_ptr<entity::EntityManager> g_entityManager;
		
		EntityManager& getEntityManager() {
			auto entityPtr = g_entityManager.lock();
			if (!entityPtr) { throw std::runtime_error("no entity manager"); }
			return *entityPtr.get();
		}


		// class SceneGraph

		void SceneGraph::update()
		{
			auto& entityMgr = getEntityManager();
			// get the SceneNode components
			auto& nodeComponents = entityMgr.getComponentStore<SceneNode>().getComponents();
			auto& nodeItems = nodeComponents.getItems();

			// start traversal at the root node
			m_bfsQueue.empty();
			m_bfsQueue.push({
				nodeComponents.getInnerIndex(m_rootNode),
				0, 0,
				glm::dvec3{ 0.0, 0.0, 0.0 },
				glm::angleAxis(0.0f, glm::vec3{ 0.0f, 1.0f, 0.0f })
			});

			// traverse the scene graph and calculate new world positions if needed
			while (!m_bfsQueue.empty()) {
				const auto& bfs = m_bfsQueue.front();

				SceneNode& node = nodeItems[bfs.nodeIndex].component;

				// recalc world position if this, or any ancestors have moved since last frame
				bool positionDirty = node.positionDirty || bfs.ancestorPositionDirty;
				if (positionDirty) {
					node.positionWorld = bfs.translationToWorld + node.translationLocal;
					node.positionDirty = false;
				}

				// recalc world orientation if this, or any ancestors have rotated since last frame
				bool orientationDirty = node.orientationDirty || bfs.ancestorOrientationDirty;
				if (orientationDirty) {
					node.orientationWorld = glm::normalize(bfs.rotationToWorld * node.rotationLocal);
					node.orientationDirty = false;
				}

				if (node.numChildren > 0) {
					// do some house cleaning of bfs_queue, make sure size doesn't get too large
					if (m_bfsQueue.get_offset() + node.numChildren > m_bfsQueue.capacity()) {
						m_bfsQueue.reset_offset();
					}

					// add all children to the traversal queue
					ComponentId childId = node.firstChild;
					do {
						auto childInnerIndex = nodeComponents.getInnerIndex(childId);
						auto& child = nodeItems[childInnerIndex];

						m_bfsQueue.push({
							childInnerIndex,
							positionDirty, orientationDirty,
							node.positionWorld,
							node.orientationWorld
						});

						childId = child.component.nextSibling;
					} while (childId != NullId_T);
				}

				m_bfsQueue.pop();
			}
		}


		ComponentId SceneGraph::addToScene(EntityId entityId, const glm::dvec3& translationLocal,
										   const glm::quat& rotationLocal, ComponentId parentNodeId)
		{
			auto& entityMgr = getEntityManager();
			// get the SceneNode components
			auto& nodeItems = entityMgr.getComponentStore<SceneNode>().getComponents().getItems();

			// get the parent node where we're inserting this component
			if (parentNodeId == NullId_T) {
				parentNodeId = m_rootNode;
			}
			auto& parentNode = nodeItems[parentNodeId].component;

			// add a SceneNode component to the entity
			SceneNode node = {
				0,															// positionDirty
				0,															// orientationDirty
				translationLocal,											// translationLocal
				rotationLocal,												// rotationLocal
				parentNode.positionWorld + translationLocal,				// positionWorld
				glm::normalize(parentNode.orientationWorld * rotationLocal),// orientationWorld
				0,															// numChildren
				NullId_T,													// firstChild
				parentNode.firstChild,										// nextSibling
				NullId_T,													// prevSibling
				parentNodeId,												// parent
				m_sceneId													// scene
			};

			ComponentId nodeId = entityMgr.addComponentToEntity(std::move(node), entityId);

			// push new node to the front of parent't list
			// set the prevSibling of the former first child
			if (parentNode.firstChild != NullId_T) {
				nodeItems[parentNode.firstChild].component.prevSibling = nodeId;
			}
			
			// make the new node the first child of its parent
			parentNode.firstChild = nodeId;
			++parentNode.numChildren;

			return nodeId;
		}


		bool SceneGraph::removeFromScene(EntityId entityId)
		{
			//auto& node = m_nodes.getComponent(nodeId);
			//auto& parentNode = m_nodes.getComponent(node.parent);

			//// if this was the firstChild, set the new one
			//if (node.prevSibling == NullId_T) {
			//	parentNode.firstChild = node.nextSibling;
			//	m_nodes.getComponent(parentNode.firstChild).prevSibling = NullId_T;
			//}
			//// fix the node's sibling linked list
			//else {
			//	m_nodes.getComponent(node.prevSibling).nextSibling = node.nextSibling;
			//}
			//
			//--parentNode.numChildren;

			// gather the node's child tree ComponentIds breadth-first
			// this gives us a list of components to remove, plus their parent entityIds
			/*static vector<ComponentId> removeIds;
			removeIds.clear();

			auto gatherId = node.firstChild;
			while (gatherId != NullId_T) {
				auto& n = m_nodes[gatherId];
				n.
				removeIds.push_back
			}*/
			
			// remove this node
			//m_nodes.removeComponent(nodeId);

			return true;
		}


		std::vector<EntityId> SceneGraph::removeTreeFromScene(EntityId entityId)
		{
			std::vector<EntityId> removedIds;

			// TODO ...

			return removedIds;
		}


		SceneGraph::SceneGraph() :
			m_sceneId{}
		{
			m_bfsQueue.reserve(RESERVE_SCENEGRAPH_TRAVERSAL_QUEUE);

			// add the root scene node
			auto& entityMgr = getEntityManager();
			
			// TEMP, should be able to create entity without mask
			auto entityId = entityMgr.createEntity();

			m_rootNode = addToScene(entityId,
									glm::dvec3{ 0.0, 0.0, 0.0 },
									glm::angleAxis(0.0f, glm::vec3{ 0.0f, 1.0f, 0.0f }),
									NullId_T);
		}
		

		SceneGraph::~SceneGraph() {
			if (m_bfsQueue.capacity() > RESERVE_SCENEGRAPH_TRAVERSAL_QUEUE) {
				SDL_Log("check RESERVE_SCENEGRAPH_TRAVERSAL_QUEUE: original=%d, highest=%d", RESERVE_SCENEGRAPH_TRAVERSAL_QUEUE, m_bfsQueue.capacity());
			}
		}


		// class SceneManager

		SceneManager::SceneManager() :
			m_scenes(0, RESERVE_SCENEMANAGER_SCENES)
		{}

		SceneManager::~SceneManager() {
			if (m_scenes.capacity() > RESERVE_SCENEMANAGER_SCENES) {
				SDL_Log("check RESERVE_SCENEMANAGER_SCENES: original=%d, highest=%d", RESERVE_SCENEMANAGER_SCENES, m_scenes.capacity());
			}
		}
	}
}