#include "../Scene.h"
#include <SDL_log.h>
#include <utility/memory_reserve.h>

namespace griffin {
	namespace scene {

		// class SceneGraph

		void SceneGraph::update()
		{
			// start traversal at the root node
			m_bfsQueue.empty();
			m_bfsQueue.push({
				m_nodes.getComponents().getInnerIndex(m_rootNode),
				0, 0,
				glm::dvec3{ 0.0, 0.0, 0.0 },
				glm::angleAxis(0.0f, glm::vec3{ 0.0f, 1.0f, 0.0f })
			});

			auto& nodes = m_nodes.getComponents().getItems();

			while (!m_bfsQueue.empty()) {
				const auto& bfs = m_bfsQueue.front();

				SceneNode& node = nodes[bfs.nodeIndex].component;

				// recalc world position if this, or any acestors have moved since last frame
				bool positionDirty = node.positionDirty || bfs.ancestorPositionDirty;
				if (positionDirty) {
					node.positionWorld = bfs.translationToWorld + node.translationLocal;
					node.positionDirty = false;
				}

				// recalc world orientation if this, or any acestors have rotated since last frame
				bool orientationDirty = node.orientationDirty || bfs.ancestorOrientationDirty;
				if (orientationDirty) {
					node.orientationWorld = glm::normalize(bfs.rotationToWorld * node.orientationLocal);
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
						auto childInnerIndex = m_nodes.getComponents().getInnerIndex(childId);
						auto& child = nodes[childInnerIndex];

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


		ComponentId SceneGraph::addNode(SceneNode&& node, ComponentId parentId, EntityId entityId)
		{
			if (parentId == NullId_T) {
				parentId = m_rootNode;
			}

			// push node to the front of child parent't list
			auto& parentNode = m_nodes[parentId].component;
			node.parent = parentId;
			node.nextSibling = parentNode.firstChild;

			// add the new node
			ComponentId nodeId = m_nodes.addComponent(std::forward<SceneNode>(node), entityId);
			
			// set the prevSibling of the former first child
			if (parentNode.firstChild != NullId_T) {
				m_nodes[parentNode.firstChild].component.prevSibling = nodeId;
			}
			
			// make the new node the first child of its parent
			parentNode.firstChild = nodeId;
			++parentNode.numChildren;

			return nodeId;
		}


		void SceneGraph::removeNode(ComponentId nodeId)
		{
			auto& node = m_nodes.getComponent(nodeId);
			auto& parentNode = m_nodes.getComponent(node.parent);

			// if this was the firstChild, set the new one
			if (node.prevSibling == NullId_T) {
				parentNode.firstChild = node.nextSibling;
				m_nodes.getComponent(parentNode.firstChild).prevSibling = NullId_T;
			}
			// fix the node's sibling linked list
			else {
				m_nodes.getComponent(node.prevSibling).nextSibling = node.nextSibling;
			}
			
			--parentNode.numChildren;

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
			m_nodes.removeComponent(nodeId);
		}


		SceneGraph::SceneGraph() :
			m_nodes(0, RESERVE_SCENEGRAPH_NODES)
		{
			m_bfsQueue.reserve(RESERVE_SCENEGRAPH_TRAVERSAL_QUEUE);

			// add the root scene node
			m_rootNode = m_nodes.addComponent(SceneNode{}, NullId_T);
		}
		

		SceneGraph::~SceneGraph() {
			if (m_nodes.getComponents().capacity() > RESERVE_SCENEGRAPH_NODES) {
				SDL_Log("check RESERVE_SCENEGRAPH_NODES: original=%d, highest=%d", RESERVE_SCENEGRAPH_NODES, m_nodes.getComponents().capacity());
			}
			if (m_bfsQueue.capacity() > RESERVE_SCENEGRAPH_TRAVERSAL_QUEUE) {
				SDL_Log("check RESERVE_SCENEGRAPH_TRAVERSAL_QUEUE: original=%d, highest=%d", RESERVE_SCENEGRAPH_TRAVERSAL_QUEUE, m_bfsQueue.capacity());
			}
		}

	}
}