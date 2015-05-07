#pragma once
#ifndef GRIFFIN_INPUT_SYSTEM_API_H_
#define GRIFFIN_INPUT_SYSTEM_API_H_

#include <utility/export.h>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// begin inclusion in lua FFI declaration
#define ffi
#ifdef ffi

	//////////
	// Input mappings map raw input events or position data to Actions, States or Ranges. These
	// are high-level game input events obtained from configuration data through Lua.
	//	Actions: Single-time events, not affected by key repeat, mapped to a single input event
	//			 e.g. {keydown-G} or {keyup-G}.
	//	States:  Binary on/off flag mapped to two input events e.g. {on:keydown-G, off:keyup-G}
	//			 or {on:keydown-G, off:keydown-G}. The first example would be typical of WASD
	//			 type movement state, the second example would behave like a toggle.
	//	Axis:	 Uses position information of joysticks, throttles, rudder pedals, head
	//			 tracking gear, even mouse movement if desired.
	//////////
	typedef struct {
		uint8_t		type;				//<! type of this mapping
		uint64_t	mappingId;			//<! the id handle of this mapping
		uint8_t		bindIn;				//<! event to start the action or state
		uint8_t		bindOut;			//<! event to end the state
		uint8_t		curve;				//<! curve type of axis
		uint32_t	instanceId;			//<! instanceID of the device, comes through event "which"
		uint32_t	button;				//<! keyboard virtual key code , mouse or joystick button
		uint16_t	modifier;			//<! keyboard modifier, SDL_Keymod, defaults to 0 (KMOD_NONE)
		uint8_t		mouseWheel;			//<! 0=false, 1=true is a mouse wheel binding
		uint8_t		axis;				//<! index of the joystick axis
		uint8_t		clicks;				//<! number of mouse clicks for binding (2==double-click)
		uint8_t		deadzone;			//<! deadzone for axis
		uint8_t		curvature;			//<! curvature of axis
		uint8_t		saturationX;		//<! saturation of the x axis
		uint8_t		saturationY;		//<! saturation of the y axis
		uint8_t		numAxes;			//<! 1 for most joysticks, 2 for mouse and balls
		uint8_t		relativeMotion;		//<! 0=false, 1=true motion is relative not absolute
		uint8_t		invert;				//<! 0=false, 1=true axis is inverted
		uint8_t		slider;				//<! 0=false, 1=true axis is a slider
		char		name[32];			//<! display name of the mapping
	} griffin_InputMapping;

	GRIFFIN_EXPORT
	uint64_t griffin_input_createContext(uint8_t priority, bool makeActive);

	GRIFFIN_EXPORT
	uint64_t griffin_input_createInputMapping(const griffin_InputMapping* mapping, uint64_t context);

#endif ffi
#undef ffi
// end inclusion in lua FFI declaration

#ifdef __cplusplus
}
#endif

#endif