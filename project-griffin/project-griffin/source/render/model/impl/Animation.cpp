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


struct NodeTransformResult {
	glm::vec3 translation;
	glm::quat rotation;
	glm::vec3 scaling;
};


NodeTransformResult getNodeTransformForTrack(const MeshAnimations& animations, AnimationTrack& track, NodeAnimation& nodeAnim, float animTime)
{
	using namespace glm;

	NodeTransformResult result{};

	// This code requires at least one key from each of position, rotation and scaling so we avoid
	// having to decompose the default node matrix. Luckily it appears that assimp always gives us
	// at least one key for each channel, but that could also be from Blender specifically. This
	// assertion tells us if there is a missing channel in the animation.
	assert(nodeAnim.numPositionKeys > 0 && nodeAnim.numRotationKeys > 0 && nodeAnim.numScalingKeys > 0 &&
		   "animation requires at least one key per channel");

	// Translation keyframes
	{
		int key1 = -1;
		int key2 = -1;
		
		// get nearest two key frames
		for (uint32_t k = nodeAnim.positionKeysIndexOffset; k < nodeAnim.positionKeysIndexOffset + nodeAnim.numPositionKeys; ++k) {
			auto& posKey = animations.positionKeys[k];
			if (animTime < posKey.time) {
				key1 = (k == nodeAnim.positionKeysIndexOffset ? k : k - 1);
				key2 = k;
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
		result.translation = mix(pos1, pos2, interp);
	}
	// Rotation keyframes
	{
		int key1 = -1;
		int key2 = -1;

		// get nearest two key frames
		for (uint32_t k = nodeAnim.rotationKeysIndexOffset; k < nodeAnim.rotationKeysIndexOffset + nodeAnim.numRotationKeys; ++k) {
			auto& rotKey = animations.rotationKeys[k];
			if (animTime < rotKey.time) {
				key1 = (k == nodeAnim.rotationKeysIndexOffset ? k : k - 1);
				key2 = k;
				break;
			}
		}

		// went past the last key
		if (key1 == -1) {
			key1 = nodeAnim.rotationKeysIndexOffset + nodeAnim.numRotationKeys - 1;
			key2 = key1;
		}

		// TODO: look at pre/post state, we may be able to exit early and accept the default modelToWorld when key1 == key2, depending on the state
		// Also, the key1 or key2 at either end of the animation may have to be set to default node transform instead of clamping the animations frame
		float time1 = animations.rotationKeys[key1].time;
		quat rot1(animations.rotationKeys[key1].w, animations.rotationKeys[key1].x, animations.rotationKeys[key1].y, animations.rotationKeys[key1].z);
		float time2 = animations.rotationKeys[key2].time;
		quat rot2(animations.rotationKeys[key2].w, animations.rotationKeys[key2].x, animations.rotationKeys[key2].y, animations.rotationKeys[key2].z);

		float interp = 0.0f;
		if (key1 != key2) {
			interp = (animTime - time1) / (time2 - time1);
		}

		// TODO: allow interpolation curves other than linear... hermite, cubic, spring system, etc.
		result.rotation = slerp(rot1, rot2, interp);
	}
	// Scaling keyframes
	{
		int key1 = -1;
		int key2 = -1;

		// get nearest two key frames
		for (uint32_t k = nodeAnim.scalingKeysIndexOffset; k < nodeAnim.scalingKeysIndexOffset + nodeAnim.numScalingKeys; ++k) {
			auto& scaleKey = animations.scalingKeys[k];
			if (animTime < scaleKey.time) {
				key1 = (k == nodeAnim.scalingKeysIndexOffset ? k : k - 1);
				key2 = k;
				break;
			}
		}

		// went past the last key
		if (key1 == -1) {
			key1 = nodeAnim.scalingKeysIndexOffset + nodeAnim.numScalingKeys - 1;
			key2 = key1;
		}

		// TODO: look at pre/post state, we may be able to exit early and accept the default modelToWorld when key1 == key2, depending on the state
		// Also, the key1 or key2 at either end of the animation may have to be set to default node transform instead of clamping the animations frame
		float time1 = animations.scalingKeys[key1].time;
		vec3 scale1(animations.scalingKeys[key1].x, animations.scalingKeys[key1].y, animations.scalingKeys[key1].z);
		float time2 = animations.scalingKeys[key2].time;
		vec3 scale2(animations.scalingKeys[key2].x, animations.scalingKeys[key2].y, animations.scalingKeys[key2].z);

		float interp = 0.0f;
		if (key1 != key2) {
			interp = (animTime - time1) / (time2 - time1);
		}

		// TODO: allow interpolation curves other than linear... hermite, cubic, spring system, etc.
		result.scaling = mix(scale1, scale2, interp);
	}
	return result;
}


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
							auto animTransform = getNodeTransformForTrack(mesh.getAnimations(), track, nodeAnim, animTime);
						}
					}
				}

				// blend this nodes's contributions from all animation tracks
				//  if total weight < 1, contribute lerp the remainder from the default transform
				//  if total weight > 1, normalize the individual weights by total weight and then blend

				// after blending collecting all animation transforms
				nodeAnimCmp.prevTranslationLocal = nodeAnimCmp.nextTranslationLocal;
				//nodeAnimCmp.nextTranslationLocal = // new blended node transform, or default
				
				
				// TODO: this should probably go into the render function???
				/*
				// compose the transform matrix
				nodeTransform = mat4_cast(nodeRotation);
				nodeTransform[3].xyz = nodeTranslation;
				nodeTransform = scale(nodeTransform, nodeScale);
				*/
			}
		}

	}

}
