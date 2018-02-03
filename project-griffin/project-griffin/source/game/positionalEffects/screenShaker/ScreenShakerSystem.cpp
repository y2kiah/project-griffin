#include "ScreenShakerSystem.h"
#include "ScreenShakerComponents.h"
#include <scene/Scene.h>
#include <entity/EntityManager.h>
//#include <input/InputSystem.h>
#include <game/impl/GameImpl.h>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/noise.hpp>

using namespace glm;
using namespace griffin;
using namespace griffin::game;

using griffin::scene::SceneNode;


void ScreenShakerSystem::updateFrameTick(
	Game& game,
	Engine& engine,
	const UpdateInfo& ui)
{
	auto& scene = engine.sceneManager->getScene(game.sceneId);
	auto& entityMgr = *scene.entityManager;

	auto& producers = entityMgr.getComponentStore<ScreenShakeProducer>();
	auto& shakeNodes = entityMgr.getComponentStore<ScreenShakeNode>();

	// for each ScreenShakeNode (receiver)
	for (auto& n : shakeNodes.getComponents()) {
		auto& shakeNode = n.component;
		auto& shakeSceneNode = entityMgr.getComponent<SceneNode>(shakeNode.sceneNodeId);

		shakeNode.prevTurbulence = shakeNode.nextTurbulence;
		shakeNode.nextTurbulence = 0;

		shakeNode.prevMaxAngle = shakeNode.nextMaxAngle;
		shakeNode.nextMaxAngle = 0;

		float shakeFreqHz = 0;

		// for each ScreenShakeProducer component, total turbulence, angle, freq for the receiver
		for (auto& p : producers.getComponents()) {
			auto& producer = p.component;
			auto& producerSceneNode = entityMgr.getComponent<SceneNode>(producer.sceneNodeId);

			float producerRadiusSq = producer.radius * producer.radius;

			vec3 nodeToProducer = shakeSceneNode.positionWorld - producerSceneNode.positionWorld;
			float distSq = dot(nodeToProducer, nodeToProducer);

			// determine the effectiveness based on distance ratio to radius of the producer
			float effectiveTurbulence = (producerRadiusSq == 0.0f)
				? producer.turbulence
				: producer.turbulence * glm::max((producerRadiusSq - distSq) / producerRadiusSq, 0.0f);

			// accumulate effective turbulence level from all producers
			shakeNode.nextTurbulence += effectiveTurbulence;

			// take the max angle and shake freq encountered
			shakeNode.nextMaxAngle = (producer.maxAngle > shakeNode.nextMaxAngle
										? producer.maxAngle : shakeNode.nextMaxAngle);

			shakeFreqHz = (producer.shakeFreqHz > shakeFreqHz ? producer.shakeFreqHz : shakeFreqHz);
		}

		// set noise time interpolation values
		shakeNode.prevNoiseTime = shakeNode.nextNoiseTime;
		if (shakeNode.prevNoiseTime > 256.0f) {
			shakeNode.prevNoiseTime -= 256.0f;
		}
		shakeNode.nextNoiseTime = shakeNode.prevNoiseTime + (ui.deltaT * shakeFreqHz);
	}

	// for each ScreenShakeProducer component
	for (auto& p : producers.getComponents()) {
		auto& producer = p.component;

		// decrease the turbulence linearly by time, unless TTL is 0 (which means it stays constant forever, probably controlled externally)
		float turbulenceFalloff = (producer.totalTimeToLiveMS == 0) ? 0
			: ui.deltaMs / producer.totalTimeToLiveMS;

		producer.turbulence -= turbulenceFalloff;
	}
	
	// remove expired components (consider adding an autoremove flag to control this)
	entityMgr.removeComponent_if<ScreenShakeProducer>([](const ScreenShakeProducer& producer) {
		return (producer.turbulence <= 0.0f);
	});
}


void ScreenShakerSystem::renderFrameTick(
	Game& game,
	Engine& engine,
	float interpolation,
	const int64_t realTime,
	const int64_t countsPassed)
{
	auto& scene = engine.sceneManager->getScene(game.sceneId);
	auto& entityMgr = *scene.entityManager;

	auto& shakeNodes = entityMgr.getComponentStore<ScreenShakeNode>();

	// for each ScreenShakeNode (receiver)
	for (auto& n : shakeNodes.getComponents()) {
		auto& shakeNode = n.component;

		float turbulence = glm::mix(shakeNode.prevTurbulence, shakeNode.nextTurbulence, interpolation);
		float turbSq = turbulence * turbulence;

		float noiseTime = glm::mix(shakeNode.prevNoiseTime, shakeNode.nextNoiseTime, interpolation);
		float maxAngle  = glm::mix(shakeNode.prevMaxAngle, shakeNode.nextMaxAngle, interpolation);
		
		float mult = glm::radians(maxAngle) * turbSq; // deg to radians

		float yawAngle   = mult * glm::perlin(vec2(noiseTime, 0.0f));
		float pitchAngle = mult * glm::perlin(vec2(noiseTime, 11.0f));
		float rollAngle  = mult * glm::perlin(vec2(noiseTime, 23.0f));

		// cameraMovementNode is the base camera transform without shake applied, so every frame we
		// rebase the shake scene node on it so the shake doesn't wander away from view direction
		auto* pCamInst = entityMgr.getEntityComponent<scene::CameraInstance>(n.entityId);
		assert(pCamInst != nullptr && "a screen shake entity must contain a camera instance");
		
		auto& camMove = entityMgr.getComponent<scene::MovementComponent>(pCamInst->movementId);
		auto& camMoveNode = entityMgr.getComponent<SceneNode>(camMove.sceneNodeId);
		auto& shakeSceneNode = entityMgr.getComponent<SceneNode>(shakeNode.sceneNodeId);
		assert(camMove.sceneNodeId != shakeNode.sceneNodeId && "camera movement shake SceneNode should be a child of the movement SceneNode, not the same node");
		
		dvec3 angles(pitchAngle, yawAngle, rollAngle);
		
		shakeSceneNode.rotationLocal = dquat(angles);
		shakeSceneNode.orientationDirty = 1;
	}
}


void ScreenShakerSystem::init(
	Game& game,
	const Engine& engine,
	const SDLApplication& app)
{
	using namespace griffin::scene;
	auto& scene = engine.sceneManager->getScene(game.sceneId);

	//playerfpsInputContextId = game.player.playerfpsInputContextId;

/*
	// get playerfps input mapping ids
	{
		auto ctx = engine.inputSystem->getInputContextHandle("playerfps");
		playerfpsInputContextId = ctx;

		lookYId = engine.inputSystem->getInputMappingHandle("Mouse Look Y", ctx);

		assert(forwardId != NullId_T && backId != NullId_T &&
			leftId != NullId_T && rightId != NullId_T &&
			sprintId != NullId_T && walkId != NullId_T &&
			crouchId != NullId_T &&
			lookXId != NullId_T && lookYId != NullId_T &&
			"playerfps input mappings changed");
	}

	// playerfps input handlers
	{
		using namespace input;

		// set the callback priority to the same as input context priority
		int priority = engine.inputSystem->getContext(playerfpsInputContextId).priority;

		engine.inputSystem->registerCallback(priority, [&engine, &game, this](FrameMappedInput& mi) {

			auto& scene = engine.sceneManager->getScene(game.sceneId);

			engine.inputSystem->handleInputState(forwardId, mi, [this](MappedState& ms, InputContext& c) {
				moveForward = glm::min(moveForward + 1, 1);
				return true;
			});
		});
	}
*/
}


ComponentId griffin::game::addScreenShakeNodeToCamera(
	scene::Scene& scene,
	EntityId entityId,
	ComponentId cameraInstanceId)
{
	auto& entityMgr = *scene.entityManager;
	auto& camInst = entityMgr.getComponent<scene::CameraInstance>(cameraInstanceId);

	// create a new scene node with a parent of the original camera node
	auto shakeSceneNodeId = scene.sceneGraph->addToScene(entityId, {}, {}, camInst.sceneNodeId);

	// add a shakenode component to control the new scene node
	game::ScreenShakeNode shakeNode{};
	shakeNode.sceneNodeId = shakeSceneNodeId;
	
	auto shakeNodeId = entityMgr.addComponentToEntity(std::move(shakeNode), entityId);

	// put the camera onto the new shakable scene node, the camera's movement component remains
	// unchanged and still points to the original scene node
	camInst.sceneNodeId = shakeSceneNodeId;

	return shakeNodeId;
}