local ffi = require("ffi")
local C = ffi.C

local gameScene = nil
local devCamera = nil --TEMP, move to devcamera system later

function initGame()
	-- set up game scene
	gameScene = C.griffin_scene_createScene("Game World", true)
	
	local devCameraParams = ffi.new("griffin_CameraParameters", {
		0.1, 100000.0,				-- near/far clip
		0, 0,						-- viewport
		60.0, C.CAMERA_PERSPECTIVE	-- fov
	})
	devCamera = C.griffin_scene_createCamera(gameScene, 0, devCameraParams, "devcamera")

	-- start up active game input contexts
	InputSystem:setContextActive("ingame")
	--InputSystem:setContextActive("playerfps")
	InputSystem:setContextActive("devcamera")
	
	-- add game input handlers
	InputSystem:addInputHandler(ingameInputHandler)
	InputSystem:addInputHandler(devcameraInputHandler)
	--InputSystem:setRelativeMouseMode(true)

end


-- handle dev camera controls
-- TODO: this will go into a GameSystem
function devcameraInputHandler(frameInput)
	InputSystem:handleInput("devcamera", "Move Forward", frameInput.states, function(context, mappedState)
		
		return true
	end)

	InputSystem:handleInput("devcamera", "Move Backward", frameInput.states, function(context, mappedState)
		
		return true
	end)

	InputSystem:handleInput("devcamera", "Move Left", frameInput.states, function(context, mappedState)
		
		return true
	end)

	InputSystem:handleInput("devcamera", "Move Right", frameInput.states, function(context, mappedState)
		
		return true
	end)

	InputSystem:handleInput("devcamera", "High Speed", frameInput.states, function(context, mappedState)
		
		return true
	end)

	InputSystem:handleInput("devcamera", "Mouse Look X", frameInput.axes, function(context, mappedAxis)
		print("devcamera axis " .. ffi.string(mappedAxis.inputMapping.name) ..
				" handled motion, relRaw=" .. mappedAxis.axisMotion.relRaw ..
				", relMapped=" .. mappedAxis.axisMotion.relMapped)
		
		return true
	end)

	InputSystem:handleInput("devcamera", "Mouse Look Y", frameInput.axes, function(context, mappedAxis)
		print("devcamera axis " .. ffi.string(mappedAxis.inputMapping.name) ..
				" handled motion, relRaw=" .. mappedAxis.axisMotion.relRaw ..
				", relMapped=" .. mappedAxis.axisMotion.relMapped)
		
		return true
	end)
end


-- handle ingame actions including Pause, Capture Mouse
function ingameInputHandler(frameInput)
	InputSystem:handleInput("ingame", "Capture Mouse", frameInput.actions, function(context, mappedAction)
		local relative = InputSystem:getRelativeMouseMode()
		InputSystem:setRelativeMouseMode(not relative)
		return true
	end)
end