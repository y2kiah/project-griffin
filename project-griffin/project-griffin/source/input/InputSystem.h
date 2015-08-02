/**
* InputMappings 
*/
#pragma once
#ifndef GRIFFIN_INPUTSYSTEM_H_
#define GRIFFIN_INPUTSYSTEM_H_

#include <vector>
#include <string>
#include <bitset>
#include <SDL_events.h>
#include <memory>
#include <application/UpdateInfo.h>
#include <utility/container/concurrent_queue.h>
#include <utility/enum.h>
#include <utility/container/handle_map.h>
#include <utility/concurrency.h>
#include <utility/memory_reserve.h>


using std::vector;
using std::bitset;
using std::tuple;
using std::string;

class SDLApplication;


namespace griffin {
	namespace input {

		// Forward declarations

		class InputSystem;
		class InputContext;

		// Typedefs/Enums

		typedef std::shared_ptr<InputSystem> InputSystemPtr;
		typedef std::weak_ptr<InputSystem>   InputSystemWeakPtr;

		// Function declarations

		extern void setInputSystemPtr(const InputSystemPtr& inputPtr);


		// Type declarations

		/**
		* Input Event types
		*/
		MakeEnum(InputEventType, uint8_t,
				 (Event_Keyboard)
				 (Event_Mouse)
				 (Event_Joystick)
				 (Event_TextInput)
				 , _T);

		/**
		* Input Cursor types
		*/
		MakeEnum(InputMouseCursor, uint8_t,
				 (Cursor_Arrow)
				 (Cursor_Hand)
				 (Cursor_Wait)
				 (Cursor_IBeam)
				 (Cursor_Crosshair)
				 , _T);

		/**
		* Input Mapping types
		*/
		MakeEnum(InputMappingType, uint8_t,
				 (Action)		//<! Actions are single-time events
				 (State)		//<! States are on/off
				 (Axis)			//<! Axis are ranges of motion normalized to [-1,1]
				 , _T);

		/**
		* Input Mapping Binding types
		*/
		MakeEnum(InputMappingBindEvent, uint8_t,
				 (Bind_Down)	//<! Bind to DOWN event of keyboard key, mouse or joystick button
				 (Bind_Up)		//<! Bind to UP event of keyboard key, mouse or joystick button
				 (Bind_Toggle)	//<! Axis are ranges of motion normalized to [-1,1]
				 , _T);

		/**
		* Input Mapping Axis Curve types
		*/
		MakeEnum(InputMappingAxisCurve, uint8_t,
				 (Curve_Linear)
				 (Curve_SCurve)
				 , _T);

		/**
		* Input mappings map raw input events or position data to Actions, States or Ranges. These
		* are high-level game input events obtained from configuration data through Lua.
		*	Actions: Single-time events, not affected by key repeat, mapped to a single input event
		*			 e.g. {keydown-G} or {keyup-G}.
		*	States:  Binary on/off flag mapped to two input events e.g. {on:keydown-G, off:keyup-G}
		*			 or {on:keydown-G, off:keydown-G}. The first example would be typical of WASD
		*			 type movement state, the second example would behave like a toggle.
		*	Axis:	 Uses position information of joysticks, throttles, rudder pedals, head
		*			 tracking gear, even mouse movement if desired.
		*/
		struct InputMapping {
			InputMappingType		type = Action_T;		//<! type of this mapping
			InputMappingBindEvent	bindIn = Bind_Down_T;	//<! event to start the action or state
			InputMappingBindEvent	bindOut = Bind_Up_T;	//<! event to end the state
			InputMappingAxisCurve	curve = Curve_SCurve_T;	//<! curve type of axis
			uint32_t				device = 0;				//<! instanceID of the device, comes through event "which"
			// keyboard, mouse button, joystick button, mouse wheel events
			uint32_t				keycode = 0;			//<! keyboard virtual key code, mouse or joystick button
			uint16_t				modifier = 0;			//<! keyboard modifier, SDL_Keymod, defaults to 0 (KMOD_NONE)
			uint8_t					mouseWheel = 0;			//<! 0=false, 1=true is a mouse wheel binding
			uint8_t					clicks = 1;				//<! number of mouse clicks for binding (2==double-click)
			// mouse motion, joystick motion, joystick hat motion, ball motion events
			uint8_t					axis = 0;				//<! index of the joystick axis
			uint8_t					deadzone = 0;			//<! deadzone for axis
			uint8_t					curvature = 0;			//<! curvature of axis
			uint8_t					saturationX = 100;		//<! saturation of the x axis
			uint8_t					saturationY = 100;		//<! saturation of the y axis
			uint8_t					relativeMotion = 0;		//<! 0=false, 1=true motion is relative not absolute
			uint8_t					invert = 0;				//<! 0=false, 1=true axis is inverted
			uint8_t					slider = 0;				//<! 0=false, 1=true axis is a slider
			float					sensitivity = 1.0f;		//<! sensitivity multiplier, mainly for mouse movement in relative mode
			// all events
			Id_T					mappingId;				//<! the id handle of this mapping
			Id_T					contextId;				//<! the id handle of the context
			char					name[32];				//<! display name of the mapping
		};

		/**
		* Mapped action for a frame.
		*/
		struct MappedAction {
			Id_T					mappingId;
			const InputMapping *	inputMapping = nullptr;
			float					x       = 0;
			float					y       = 0;			//<! mouse clicks include normalized position here
			int32_t					xRaw    = 0;
			int32_t					yRaw    = 0;
			bool					handled = false;		//<! flag set to true when event has been handled by a callback
		};

		/**
		* Mapped active state for a frame. All input timings are quantized to the update frame
		* rate, therefor the first frame a state becomes active includes the full frame timestep.
		*/
		struct MappedState {
			Id_T					mappingId;
			const InputMapping *	inputMapping = nullptr;
			double					totalMs     = 0;		//<! total millis the state has been active
			int64_t					startCounts = 0;		//<! clock counts when state began
			int64_t					totalCounts = 0;		//<! currentCounts - startCounts + countsPerTick
			int64_t					startFrame  = 0;		//<! frame number when state began
			int32_t					totalFrames = 0;		//<! currentFrame - startFrame + 1
			bool					handled     = false;	//<! flag set to true when event has been handled by a callback
		};

		/**
		* Axis motion for a frame. Motion events are accumulated for the frame to get relative, and
		* the last absolute position value is taken for the frame.
		*/
		struct AxisMotion {
			uint32_t				device;					//<! instanceID of the device that owns this axis, mouse is always 0 (x) and 1 (y)
			uint32_t				axis;					//<! axis number on the device
			float					posMapped = 0;			//<! absolute position of axis mapped to curve
			float					relMapped = 0;			//<! relative motion of the axis since last frame mapped to curve
			int32_t					posRaw = 0;				//<! raw value from device, not normalized or mapped to curve, may be useful but use posMapped by default
			int32_t					relRaw = 0;				//<! relative raw value of the axis
			const char *			deviceName = nullptr;	//<! name of the device
		};

		/**
		* MappedAxis matches up AxisMotion with a valid InputMapping for a frame.
		*/
		struct MappedAxis {
			Id_T					mappingId;
			const InputMapping *	inputMapping = nullptr;
			const AxisMotion *		axisMotion = nullptr;
			bool					handled = false;		//<! flag set to true when event has been handled by a callback
			uint8_t					_padding_end[4];
		};

		/**
		* Container holding all mapped input for a frame, plus text input
		*/
		struct FrameMappedInput {
			vector<MappedAction>	actions;				//<! Actions mapped to an active InputMapping for the frame
			vector<MappedState>		states;					//<! States mapped to an active InputMapping for the frame
			vector<MappedAxis>		axes;					//<! Axes mapped to an active InputMapping for the frame
			vector<AxisMotion>		motion;					//<! Holds accumulated motion for the mouse and joysticks
			std::wstring			textInput;				//<! Text input buffer
			/*std::wstring			textComposition;		//<! Text editing buffer
			int						cursorPos = 0;			//<! Text editing cursor position
			int						selectionLength = 0;*/	//<! Text editing selection length (if any)
		};

		/**
		* Input Event
		*/
		struct InputEvent {
			int64_t					timeStampCounts;
			SDL_Event				evt;
			InputEventType			eventType;
			uint8_t					_padding_end[7];
		};

		/**
		* Active Input Context record
		*/
		struct ActiveInputContext {
			Id_T					contextId;
			bool					active;
			uint8_t					priority;
			uint8_t					_padding_end[3];
		};


		/**
		* Input System
		*/
		class InputSystem {
		public:
			// Static Variables

			static const SDLApplication* app;


			// Public Functions

			explicit InputSystem() :
				m_eventsQueue(RESERVE_INPUTSYSTEM_EVENTSQUEUE),
				m_motionEventsQueue(RESERVE_INPUTSYSTEM_MOTIONEVENTSQUEUE),
				m_inputMappings(0, RESERVE_INPUTSYSTEM_MAPPINGS),
				m_inputContexts(0, RESERVE_INPUTSYSTEM_CONTEXTS),
				m_callbacks(0, RESERVE_INPUTSYSTEM_CALLBACKS)
			{
				m_popEvents.reserve(RESERVE_INPUTSYSTEM_POPQUEUE);
				m_popMotionEvents.reserve(RESERVE_INPUTSYSTEM_MOTIONPOPQUEUE);
				m_activeInputContexts.reserve(RESERVE_INPUTSYSTEM_CONTEXTS);
				// FrameMappedInput reserves
				m_frameMappedInput.actions.reserve(RESERVE_INPUTSYSTEM_POPQUEUE);
				m_frameMappedInput.states.reserve(RESERVE_INPUTSYSTEM_POPQUEUE);
			}
			
			/**
			* Destructor frees cursors and other resources
			*/
			~InputSystem();

			/**
			* Initialize the system, detect devices, etc.
			*/
			void initialize();

			/**
			* Executed in the update fixed timestep loop
			*/
			void updateFrameTick(const UpdateInfo& ui);

			/**
			* Executed on the input/GUI thread
			*/
			bool handleEvent(const SDL_Event& event);


			// Input Mappings

			/**
			* Create an input mapping and get back its handle
			*/
			Id_T createInputMapping(InputMapping&& i)
			{
				return m_inputMappings.insert(std::forward<InputMapping>(i));
			}
			
			/**
			* Get an input mapping id from its name, systems use this at initialization and store
			* the handle for subsequently matching frame input mappings
			*/
			Id_T getInputMappingHandle(const string& name, Id_T contextId) const;
			
			/**
			* Get an input mapping from its handle, asserts that the handle is valid.
			*/
			InputMapping& getInputMapping(Id_T handle)				{ return m_inputMappings[handle]; }
			const InputMapping& getInputMapping(Id_T handle) const	{ return m_inputMappings[handle]; }

			/**
			* Get index of active state in frame states array, -1 if not present.
			*/
			size_t findActiveState(Id_T mappingId) const;


			// Input Contexts

			/**
			* Create a context and get back its handle
			*/
			Id_T createContext(uint16_t optionsMask, uint8_t priority, bool makeActive = false);

			/**
			* Set the InputContext, returns true on success
			*/
			bool setContextActive(Id_T contextId, bool active = true);

			/**
			* Get an input context id from its name, systems use this at initialization and store
			* the handle for subsequently manipulating contexts
			*/
			Id_T getInputContextHandle(const string& name) const;

			/**
			* Get an input context from its handle, asserts that the handle is valid.
			*/
			InputContext& getContext(Id_T contextId)				{ return m_inputContexts[contextId]; }
			const InputContext& getContext(Id_T contextId) const	{ return m_inputContexts[contextId]; }


			// Callbacks

			typedef std::function<void(FrameMappedInput&)> CallbackFunc_T;
			
			Id_T registerCallback(int priority, CallbackFunc_T func)
			{
				return m_callbacks.insert(std::move(func));
			}

			bool unregisterCallback(Id_T callbackId)
			{
				return (m_callbacks.erase(callbackId) == 1);
			}


			// Input Modes

			/**
			* Text editing mode
			*/
			void startTextInput();
			void stopTextInput();
			bool textInputActive() const;

			/**
			* Mouse movement mode
			*/
			void startRelativeMouseMode();
			void stopRelativeMouseMode();
			bool relativeMouseModeActive() const;

		private:
			// Private Functions

			/**
			* Translate input events into mapped into for one frame
			*/
			void mapFrameInputs(const UpdateInfo& ui);
			void mapFrameMotion(const UpdateInfo& ui);

			// Private Variables

			concurrent_queue<InputEvent>	m_eventsQueue;			//<! push on input thread, pop on update thread
			concurrent_queue<InputEvent>	m_motionEventsQueue;
			vector<InputEvent>				m_popEvents;			//<! pop events from the concurrent_queues into these buffers
			vector<InputEvent>				m_popMotionEvents;

			//struct ThreadSafeState {
			handle_map<InputMapping>		m_inputMappings;		//<! collection of input mappings (actions,states,axes)
			handle_map<InputContext>		m_inputContexts;		//<! collection of input contexts
			vector<ActiveInputContext>		m_activeInputContexts;	//<! active input contexts sorted by priority ascending
			handle_map<CallbackFunc_T>		m_callbacks;
			FrameMappedInput				m_frameMappedInput;		//<! per-frame mapped input buffer

			//	ThreadSafeState() : m_inputContexts(0, RESERVE_INPUT_CONTEXTS) {}
			//};
			//monitor<ThreadSafeState>		tss_;

			SDL_Cursor *					m_cursors[InputMouseCursorCount]; //<! table of mouse cursors
			vector<SDL_Joystick*>			m_joysticks;			//<! list of opened joysticks
		};


		/**
		* Input Context Options
		*/
		MakeEnum(InputContextOptions, uint8_t,
				 (CaptureTextInput)		//<! true if text input events should be captured by this context
				 (RelativeMouseMode)	//<! true for relative mouse mode vs. regular "GUI" mode
				 (ShowMouseCursor)		//<! true to show cursor specified by m_cursorIndex
				 (EatKeyboardEvents)	//<! true to eat all keyboard events, preventing pass-down to lower contexts
				 (EatMouseEvents)		//<! prevent mouse events from passing down
				 (EatJoystickEvents)	//<! prevent joystick events from passing down
				 , _T);

		/**
		* Input Context
		*/
		class InputContext {
		public:
			/**
			* Constructors
			*/
			explicit InputContext() {}

			explicit InputContext(uint16_t optionsMask) :
				options{ optionsMask }
			{
				inputMappings.reserve(RESERVE_INPUTCONTEXT_MAPPINGS);
			}

			InputContext(InputContext&& _c) _NOEXCEPT
			  :	options{ _c.options },
				inputMappings(std::move(_c.inputMappings))
			{
				_c.options = 0;
			}

			//InputContext(const InputContext&) = delete; // I want to delete this and force move semantics only, http://stackoverflow.com/questions/12251368/type-requirements-for-stdvectortype

			~InputContext();

			InputContext& operator=(InputContext&& _c)
			{
				if (this != &_c) {
					options = _c.options;
					_c.options = 0;
					inputMappings = std::move(_c.inputMappings);
				}
				return *this;
			}

			// Variables

			bitset<InputContextOptionsCount> options = {};	//<! all input context options
			Id_T			contextId;						//<! the id handle of this context
			char			name[32];						//<! display name of the context
			//uint32_t		cursorIndex;					//<! lookup into input system's cursor table
			vector<Id_T>	inputMappings = {};				//<! stores input mapping to actions, states, axes
		};

	}
}

#endif