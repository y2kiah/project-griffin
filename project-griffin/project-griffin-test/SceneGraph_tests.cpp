#include "stdafx.h"
#include "CppUnitTest.h"

#include <cstdint>
#include <climits>
#include <ostream>
#include "../project-griffin/source/scene/Scene.h"
#include "../project-griffin/source/scene/SceneGraph.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using std::ostringstream;
using std::endl;

namespace projectgriffintest
{
	using namespace griffin;
	using namespace griffin::scene;

	TEST_CLASS(SceneGraph_tests)
	{
	public:

		TEST_METHOD(sceneGraph_test)
		{
			ostringstream oss;

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

			Logger::WriteMessage(oss.str().c_str());
		}

	};
}