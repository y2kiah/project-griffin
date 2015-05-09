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
	enum {
		CONTEXT_OPTION_CAPTURE_TEXT_INPUT  = 0,		//<! true if text input events should be captured by this context
		CONTEXT_OPTION_RELATIVE_MOUSE_MODE = 1,		//<! true for relative mouse mode vs. regular "GUI" mode
		CONTEXT_OPTION_SHOW_MOUSE_CURSOR   = 2,		//<! true to show cursor specified by m_cursorIndex
		CONTEXT_OPTION_EAT_KEYBOARD_EVENTS = 4,		//<! true to eat all keyboard events, preventing pass-down to lower contexts
		CONTEXT_OPTION_EAT_MOUSE_EVENTS    = 8,		//<! prevent mouse events from passing down
		CONTEXT_OPTION_EAT_JOYSTICK_EVENTS = 16		//<! prevent joystick events from passing down
	};

	enum {
		MAPPING_TYPE_ACTION = 0,
		MAPPING_TYPE_STATE  = 1,
		MAPPING_TYPE_AXIS   = 2
	};

	enum {
		MAPPING_BIND_DOWN   = 0,
		MAPPING_BIND_UP     = 1,
		MAPPING_BIND_TOGGLE = 2
	};
	
	enum {
		MAPPING_CURVE_LINEAR = 0,
		MAPPING_CURVE_SCURVE = 1
	};

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
		uint8_t		bindIn;				//<! event to start the action or state
		uint8_t		bindOut;			//<! event to end the state
		uint8_t		curve;				//<! curve type of axis
		uint32_t	instanceId;			//<! instanceID of the device, comes through event "which"
		uint32_t	button;				//<! keyboard virtual key code, mouse or joystick button
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
		uint64_t	mappingId;			//<! the id handle of this mapping
		uint64_t	contextId;			//<! the id handle of the context
		char		name[32];			//<! display name of the mapping
	} griffin_InputMapping;

	GRIFFIN_EXPORT
	uint64_t griffin_input_createContext(uint16_t optionsMask, uint8_t priority, const char name[32], bool makeActive);

	GRIFFIN_EXPORT
	bool griffin_input_setContextActive(uint64_t context, bool active);

	GRIFFIN_EXPORT
	uint64_t griffin_input_createInputMapping(const char name[32], uint64_t context);

	GRIFFIN_EXPORT
	griffin_InputMapping* griffin_input_getInputMapping(uint64_t mapping);

	//typedef void(*CallbackFunc_T)()

	GRIFFIN_EXPORT
	void griffin_input_registerCallback();

#endif ffi
#undef ffi
// end inclusion in lua FFI declaration

#ifdef __cplusplus
}
#endif

#endif