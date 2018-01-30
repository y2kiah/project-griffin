#include <cstdint>
#include <sstream>
#include <utility/Logger.h>
#include <scene/Scene.h>
#include <scene/SceneGraph.h>

using namespace griffin;

void testSceneGraph() {
	using namespace std;

	//ostringstream oss;

	/*Scene scene("Test Scene");
	EntityId entities[10];
	SceneNodeId nodes[10];

	for (int i = 0; i < 10; ++i) {
	entities[i] = scene.entityManager.createEntity();
	}

	nodes[0] = scene.sceneGraph.addToScene(entities[0], {}, {}, NullId_T);
	std::vector<SceneNodeId> ids;
	scene.sceneGraph.collectAncestors(NullId_T, ids);
	Assert::AreEqual(static_cast<int>(ids.size()), 1, L"One node added", LINE_INFO());
	*/

	//logger.info(Logger::Category_Test, oss.str().c_str());
}