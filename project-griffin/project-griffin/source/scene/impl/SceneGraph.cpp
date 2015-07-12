/**
* @file SceneGraph.cpp
* @author Jeff Kiah
*/
#include "../SceneGraph.h"
#include <SDL_log.h>
#include <utility/memory_reserve.h>
#include <entity/EntityManager.h>

using namespace griffin::scene;


// class SceneGraph

void SceneGraph::update()
{
	// get the SceneNode components
	auto& nodeComponents = entityMgr.getComponentStore<SceneNode>().getComponents();
	auto& nodeItems = nodeComponents.getItems();

	// start traversal at the root node
	m_bfsQueue.empty();
	m_bfsQueue.push({
		&m_rootNode,
		0, 0,
		{ 0.0, 0.0, 0.0 },
		{ 0.0f, 0.0f, 0.0f, 1.0f }
	});

	// traverse the scene graph and calculate new world positions if needed
	while (!m_bfsQueue.empty()) {
		const auto& bfs = m_bfsQueue.front();

		SceneNode& node = *bfs.sceneNode;

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
			auto childId = node.firstChild;
			for (uint32_t c = 0; c < node.numChildren; ++c) {
				assert(childId != NullId_T && "expected scene node is null, broken linked list or numChildren out of sync");
				auto& child = nodeComponents[childId].component;

				m_bfsQueue.push({
					&child,
					positionDirty, orientationDirty,
					node.positionWorld,
					node.orientationWorld
				});

				childId = child.nextSibling;
			}
		}

		m_bfsQueue.pop();
	}
}


SceneNodeId SceneGraph::addToScene(EntityId entityId, const glm::dvec3& translationLocal,
									const glm::quat& rotationLocal, SceneNodeId parentNodeId)
{
	auto& nodeComponents = entityMgr.getComponentStore<SceneNode>().getComponents();

	// get the parent node where we're inserting this component
	auto& parentNode = (parentNodeId == NullId_T) ? m_rootNode : nodeComponents[parentNodeId].component;

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
		parentNodeId												// parent
	};

	auto nodeId = entityMgr.addComponentToEntity(std::move(node), entityId);

	// push new node to the front of parent't list
	// set the prevSibling of the former first child
	if (parentNode.firstChild != NullId_T) {
		nodeComponents[parentNode.firstChild].component.prevSibling = nodeId;
	}

	// make the new node the first child of its parent
	parentNode.firstChild = nodeId;
	++parentNode.numChildren;

	return nodeId;
}


bool SceneGraph::removeFromScene(SceneNodeId sceneNodeId, bool cascade, std::vector<EntityId>* removedEntities)
{
	assert(sceneNodeId.typeId == SceneNode::componentType && "component is the wrong type");
	
	auto& nodeComponents = entityMgr.getComponentStore<SceneNode>().getComponents();

	if (!nodeComponents.isValid(sceneNodeId)) {
		return false;
	}

	auto& node = nodeComponents[sceneNodeId].component;
	auto& parentNode = (node.parent == NullId_T) ? m_rootNode : nodeComponents[node.parent].component;

	// remove from scene graph
	// if this was the firstChild, set the new one
	if (node.prevSibling == NullId_T) {
		parentNode.firstChild = node.nextSibling;
		nodeComponents[parentNode.firstChild].component.prevSibling = NullId_T;
	}
	// fix the node's sibling linked list
	else {
		nodeComponents[node.prevSibling].component.nextSibling = node.nextSibling;
	}

	--parentNode.numChildren;

	if (node.numChildren > 0) {
		// remove all ancestors
		if (cascade) {
			m_handleBuffer.clear();
			collectAncestors(sceneNodeId, m_handleBuffer);
			if (removedEntities != nullptr) {
				auto& rmvEnt = *removedEntities;
				for (auto ancestorSceneNodeId : m_handleBuffer) {
					rmvEnt.push_back(nodeComponents[ancestorSceneNodeId].entityId);
					removeFromScene(ancestorSceneNodeId, false, nullptr);
				}
			}
			else {
				for (auto ancestorSceneNodeId : m_handleBuffer) {
					removeFromScene(ancestorSceneNodeId, false, nullptr);
				}
			}
		}
		// don't cascade the delete, give the node's children to its parent
		else {
			moveAllSiblings(node.firstChild, node.parent);
		}
	}

	// remove the component
	bool removed = entityMgr.removeComponentFromEntity(sceneNodeId);

	return removed;
}


bool SceneGraph::removeEntityFromScene(EntityId entityId, bool cascade, std::vector<EntityId>* removedEntities)
{
	if (!entityMgr.entityIsValid(entityId)) {
		return false;
	}

	auto& entityComponents = entityMgr.getEntityComponents(entityId);

	bool removed = false;
	for (auto &sceneNodeId : entityComponents) {
		if (sceneNodeId.typeId == SceneNode::componentType) {
			removed = removeFromScene(sceneNodeId, cascade, removedEntities);
			if (removed) { break; }
		}
	}

	return removed;
}


bool SceneGraph::moveNode(SceneNodeId sceneNodeId, SceneNodeId moveToParent)
{
	auto& nodeComponents = entityMgr.getComponentStore<SceneNode>().getComponents();

	if (!nodeComponents.isValid(sceneNodeId) || !nodeComponents.isValid(moveToParent)) {
		return false;
	}

	auto& node = nodeComponents[sceneNodeId].component;
	// check to make sure we're not trying to move into the current parent
	if (node.parent == moveToParent) {
		return false;
	}

	auto& currentParent = (node.parent == NullId_T) ? m_rootNode : nodeComponents[node.parent].component;
	auto& newParent = (moveToParent == NullId_T) ? m_rootNode : nodeComponents[moveToParent].component;

	// remove from current parrent
	// if this was the firstChild, set the new one
	if (node.prevSibling == NullId_T) {
		currentParent.firstChild = node.nextSibling;
		nodeComponents[currentParent.firstChild].component.prevSibling = NullId_T;
	}
	// fix the node's sibling linked list
	else {
		nodeComponents[node.prevSibling].component.nextSibling = node.nextSibling;
	}

	--currentParent.numChildren;

	// move to new parent
	node.parent = moveToParent;
	node.nextSibling = newParent.firstChild;
	node.prevSibling = NullId_T;
	if (newParent.firstChild != NullId_T) {
		nodeComponents[newParent.firstChild].component.prevSibling = sceneNodeId;
	}
	newParent.firstChild = sceneNodeId;
			
	++newParent.numChildren;

	return true;
}


bool SceneGraph::moveAllSiblings(SceneNodeId siblingToMove, SceneNodeId moveToParent)
{
	auto& nodeComponents = entityMgr.getComponentStore<SceneNode>().getComponents();

	if (!nodeComponents.isValid(siblingToMove) || !nodeComponents.isValid(moveToParent)) {
		return false;
	}

	auto& node = nodeComponents[siblingToMove].component;
	// check to make sure we're not trying to move into the current parent
	if (node.parent == moveToParent) {
		return false;
	}

	auto& currentParent = (node.parent == NullId_T) ? m_rootNode : nodeComponents[node.parent].component;

	bool allMoved = true;

	// move each child to the new parent
	auto childId = currentParent.firstChild;
	while (childId != NullId_T) {
		auto& child = nodeComponents[childId].component;
		allMoved = allMoved && moveNode(childId, moveToParent);
		childId = child.nextSibling;
	}

	assert(currentParent.numChildren == 0 && "numChildren > 0 but all siblings should have moved");

	return allMoved;
}


SceneNodeId SceneGraph::getLastImmediateChild(SceneNodeId sceneNodeId) const
{
	auto& nodeComponents = entityMgr.getComponentStore<SceneNode>().getComponents();

	if (!nodeComponents.isValid(sceneNodeId)) {
		return NullId_T;
	}

	auto& node = nodeComponents[sceneNodeId].component;
	auto childId = node.firstChild;
	if (childId != NullId_T) {
		for (;;) {
			auto& child = nodeComponents[sceneNodeId].component;
			if (child.nextSibling == NullId_T) {
				break;
			}
			childId = child.nextSibling;
		}
	}

	return childId;
}


void SceneGraph::collectAncestors(SceneNodeId sceneNodeId, std::vector<SceneNodeId>& outAncestors) const
{
	auto& nodeComponents = entityMgr.getComponentStore<SceneNode>().getComponents();

	vector_queue<SceneNodeId> bfsQueue;
	auto& node = nodeComponents[sceneNodeId].component;
	
	bfsQueue.push(sceneNodeId);

	while (!bfsQueue.empty()) {
		SceneNodeId thisId = bfsQueue.front();

		auto& node = nodeComponents[thisId].component;
		auto childId = node.firstChild;
		for (uint32_t c = 0; c < node.numChildren; ++c) {
			auto& child = nodeComponents[childId].component;
			outAncestors.push_back(childId);
			bfsQueue.push(childId);
			childId = child.nextSibling;
		}

		bfsQueue.pop();
	}
}


SceneGraph::SceneGraph(EntityManager& _entityMgr) :
	entityMgr{ _entityMgr },
	m_sceneId{},
	m_rootNode{}
{
	m_bfsQueue.reserve(RESERVE_SCENEGRAPH_TRAVERSAL_QUEUE);
	m_handleBuffer.reserve(RESERVE_SCENEGRAPH_TRAVERSAL_QUEUE);

	m_rootNode.rotationLocal.w = 1.0f;
	m_rootNode.orientationWorld.w = 1.0f;
}
		

SceneGraph::~SceneGraph() {
	if (m_bfsQueue.capacity() > RESERVE_SCENEGRAPH_TRAVERSAL_QUEUE) {
		SDL_Log("check RESERVE_SCENEGRAPH_TRAVERSAL_QUEUE: original=%d, highest=%d", RESERVE_SCENEGRAPH_TRAVERSAL_QUEUE, m_bfsQueue.capacity());
	}
}
