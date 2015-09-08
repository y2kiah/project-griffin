#include <render/RenderComponents.h>
#include <render/Render.h>
//#include <render/ModelManager_GL.h>
#include <scene/SceneGraph.h>
#include <entity/EntityManager.h>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <render/model/Model_GL.h>


using namespace griffin;
using namespace griffin::render;


static Model_GL g_tempModel;


// TODO: make sure animation takes place only when instance is going to be visible after early frustum cull
void updateMeshInstanceAnimations(entity::EntityManager& entityMgr)
{
	using namespace glm;

	auto& animationStore = entityMgr.getComponentStore<MeshAnimationTrackComponent>();

	// for each animated mesh instance
	for (auto& animInst : animationStore.getComponents().getItems()) {
		auto& meshInst = entityMgr.getComponent<scene::ModelInstanceContainer>(animInst.entityId);
		auto& mesh = g_tempModel.m_mesh; // TODO: don't use the global model, get model from resource system??

		// we're handling a range of up to six animations, there may be more or less
		uint32_t firstAnim = animInst.component.baseAnimationIndex;
		uint32_t lastAnim = min(mesh.getAnimations().numAnimationTracks, firstAnim + 6);

		// for all node animation components in this mesh instance
		for (auto cmpId : entityMgr.getAllEntityComponents(animInst.entityId)) {
			if (cmpId.typeId == MeshNodeAnimationComponent::componentType) {
				auto& nodeAnimCmp = entityMgr.getComponent<MeshNodeAnimationComponent>(cmpId);

				// for each animation track, and each node animation that targets this node
				for (uint32_t a = firstAnim; a < lastAnim; ++a) {
					float animTime = animInst.component.nextAnimationTime[a];
					// if blend > 0

					auto& track = mesh.getAnimations().animations[a];
					for (uint32_t na = track.nodeAnimationsIndexOffset; na < track.nodeAnimationsIndexOffset + track.numNodeAnimations; ++na) {
						auto& nodeAnim = mesh.getAnimations().nodeAnimations[na];
						if (nodeAnim.sceneNodeIndex == nodeAnimCmp.nodeIndex) {
							//getNodeTransformForTrack(mesh.getAnimations(), track, nodeAnim, animTime);
						}
					}
				}
			}
		}

	}

}


void getNodeTransformForTrack(const MeshAnimations& animations, AnimationTrack& track, NodeAnimation& nodeAnim, float animTime)
{
	using namespace glm;

	if (nodeAnim.numPositionKeys > 0) {
		int key1 = -1;
		int key2 = -1;
		// get nearest two key frames
		for (uint32_t pk = nodeAnim.positionKeysIndexOffset; pk < nodeAnim.positionKeysIndexOffset + nodeAnim.numPositionKeys; ++pk) {
			auto& posKey = animations.positionKeys[pk];
			if (animTime < posKey.time) {
				key1 = (pk == nodeAnim.positionKeysIndexOffset ? pk : pk - 1);
				key2 = pk;
				break;
			}
		}
		// went past the last key
		if (key1 == -1) {
			key1 = nodeAnim.positionKeysIndexOffset + nodeAnim.numPositionKeys - 1;
			key2 = key1;
		}

		// TODO: look at pre/post state, we may be able to exit early and accept the default modelToWorld when key1 == key2, depending on the state
		// Also, the key1 or key2 at either end of the animation may have to be set to default node transform instead of clamping the animations frame
		float time1 = animations.positionKeys[key1].time;
		vec3 pos1(animations.positionKeys[key1].x, animations.positionKeys[key1].y, animations.positionKeys[key1].z);
		float time2 = animations.positionKeys[key2].time;
		vec3 pos2(animations.positionKeys[key2].x, animations.positionKeys[key2].y, animations.positionKeys[key2].z);

		float interp = 0.0f;
		if (key1 != key2) {
			interp = (animTime - time1) / (time2 - time1);
		}

		// TODO: allow interpolation curves other than linear... hermite, cubic, spring system, etc.
		pos1 = mix(pos1, pos2, interp);
//		nodeTransform[3].xyz = pos1;
// TODO: add to return
	}
}