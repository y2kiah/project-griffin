local ffi = require("ffi")

ffi.cdef[[
	enum { DevCameraMovementComponentTypeId = 0 };

	typedef struct {
		float	moveForward;
		float	moveSide;
		float	moveVertical;
		bool	highSpeed;
	} DevCameraMovementComponent;
]]
local C = ffi.C

local DevCameraMovementComponent = ffi.typeof("DevCameraMovementComponent");
local devCameraMovementComponentStoreId = nil

local gameScene = nil

--TEMP, move these to devcamera system later
local devCamera = nil
local devCameraMovement = nil

function initGame()
	-- set up game scene
--[[	gameScene = C.griffin_scene_createScene("Game World", true)
	
	-- create game component stores for this scene
	devCameraMovementComponentStoreId = C.griffin_scene_createComponentStore(
											gameScene,
											C.DevCameraMovementComponentTypeId,
											ffi.sizeof(DevCameraMovementComponent),
											1)

	-- create game scene cameras
	local devCameraParams = ffi.new("griffin_CameraParameters", {
		0.1, 100000.0,				-- near/far clip
		0, 0,						-- viewport
		60.0, C.CAMERA_PERSPECTIVE	-- fov
	})
	devCamera = C.griffin_scene_createCamera(gameScene, 0, devCameraParams, "devcamera")
	devCameraMovement = C.griffin_scene_addComponentToEntity(gameScene, C.DevCameraMovementComponentTypeId, devCamera)
]]
	-- start up active game input contexts
	InputSystem:setContextActive("ingame")
	--InputSystem:setContextActive("playerfps")
--	InputSystem:setContextActive("devcamera")
	
	-- add game input handlers
	InputSystem:addInputHandler(ingameInputHandler)
--	InputSystem:addInputHandler(devcameraInputHandler)
	--InputSystem:setRelativeMouseMode(true)
end


-- handle dev camera controls
-- TODO: this will go into a GameSystem
function devcameraInputHandler(frameInput)
	local devCamMove = ffi.cast("DevCameraMovementComponent*", C.griffin_scene_getComponentData(gameScene, devCameraMovement))

	InputSystem:handleInput("devcamera", "Move Forward", frameInput.states, function(context, mappedState)
		devCamMove.moveForward = devCamMove.moveForward + 1.0
		return true
	end)

	InputSystem:handleInput("devcamera", "Move Backward", frameInput.states, function(context, mappedState)
		devCamMove.moveForward = devCamMove.moveForward - 1.0
		return true
	end)

	InputSystem:handleInput("devcamera", "Move Left", frameInput.states, function(context, mappedState)
		devCamMove.moveSide = devCamMove.moveSide - 1.0
		return true
	end)

	InputSystem:handleInput("devcamera", "Move Right", frameInput.states, function(context, mappedState)
		devCamMove.moveSide = devCamMove.moveSide + 1.0
		return true
	end)

	InputSystem:handleInput("devcamera", "Move Up", frameInput.states, function(context, mappedState)
		devCamMove.moveVertical = devCamMove.moveVertical + 1.0
		return true
	end)

	InputSystem:handleInput("devcamera", "Move Down", frameInput.states, function(context, mappedState)
		devCamMove.moveVertical = devCamMove.moveVertical - 1.0
		return true
	end)

	InputSystem:handleInput("devcamera", "High Speed", frameInput.states, function(context, mappedState)
		devCamMove.highSpeed = true
		return true
	end)

	InputSystem:handleInput("devcamera", "Mouse Look X", frameInput.axes, function(context, mappedAxis)
		--[[print("devcamera axis " .. ffi.string(mappedAxis.inputMapping.name) ..
				" handled motion, relRaw=" .. mappedAxis.axisMotion.relRaw ..
				", relMapped=" .. mappedAxis.axisMotion.relMapped)
		]]
		return true
	end)

	InputSystem:handleInput("devcamera", "Mouse Look Y", frameInput.axes, function(context, mappedAxis)
		--[[print("devcamera axis " .. ffi.string(mappedAxis.inputMapping.name) ..
				" handled motion, relRaw=" .. mappedAxis.axisMotion.relRaw ..
				", relMapped=" .. mappedAxis.axisMotion.relMapped)
		]]
		return true
	end)
end


function devcameraUpdate()
	local devCamMove = ffi.cast("DevCameraMovementComponent*", C.griffin_scene_getComponentData(gameScene, devCameraMovement))
	-- use this frame's movement values to accelerate the camera


	-- reset movement values for next frame
	devCamMove.moveForward = 0
	devCamMove.moveSide = 0
	devCamMove.moveVertical = 0
	devCamMove.highSpeed = false
end



-- handle ingame actions including Pause, Capture Mouse
function ingameInputHandler(frameInput)
	InputSystem:handleInput("ingame", "Capture Mouse", frameInput.actions, function(context, mappedAction)
		local relative = InputSystem:getRelativeMouseMode()
		InputSystem:setRelativeMouseMode(not relative)
		return true
	end)
end