#pragma once
#ifndef GRIFFIN_GAME_SCREENSHAKERCOMPONENTS_H_
#define GRIFFIN_GAME_SCREENSHAKERCOMPONENTS_H_

#include <entity/components.h>
#include <scene/Scene.h>
#include <entity/EntityTypedefs.h>


namespace griffin {
	namespace game {
		using scene::SceneNodeId;
		using entity::EntityId;
		using entity::ComponentId;


		/**
		* Causes shake on nearby ScreenShakeNodes
		*/
		COMPONENT(ScreenShakeProducer,
			(SceneNodeId,	sceneNodeId,,		"center position of the shake producer"),
			(float,			startTurbulence,,	"starting strength of screen shake, 0=none, 0.25=light, 0.75=heavy, 1.0=severe"),
			(float,			turbulence,,		"current turbulence level"),
			(float,			totalTimeToLiveMS,,	"total time the shaker is active, turbulence goes from startTurbulence to 0 linearly over this time"),
			(float,			radius,,			"radius of the effective area in feet"),
			(float,			shakeFreqHz,,		"frequency of shake oscillations in Hz, somewhere in the ballpark of 10.0 tends to work well"),
			(float,			maxAngle,,			"max possible shake angle for yaw, pitch and roll")
		)

		/**
		* Pairs with a parent SceneNode and receives shake from nearby ScreenShakeProducers.
		*/
		COMPONENT(ScreenShakeNode,
			(SceneNodeId,	sceneNodeId,,		"scene node for the camera instance to base shake angles on"),
			(float,			prevTurbulence,,	"previous effective turbulence used for interpolation"),
			(float,			nextTurbulence,,	"next effective turbulence used for interpolation"),
			(float,			prevNoiseTime,,		"previous time for perlin noise used for interpolation"),
			(float,			nextNoiseTime,,		"next time for perlin noise used for interpolation"),
			(float,			prevMaxAngle,,		"previous max shake angle in degrees used for interpolation"),
			(float,			nextMaxAngle,,		"next max shake angle in degrees used for interpolation")
		)


		/**
		* Makes a camera shakable by adding a ScreenShakeNode and SceneNode to the CameraInstance
		* entity. Sets the camera instance scene node to point to the newly created shake-enabled
		* node. If the camera already contains a ScreenShakeNode, nothing is added and the
		* existing component id is returned.
		* 
		* @return  id of the ScreenShakeNode component
		*/
		ComponentId addScreenShakeNodeToCamera(
						scene::Scene& scene,
						EntityId entityId,
						ComponentId cameraInstanceId);
	}
}

#endif