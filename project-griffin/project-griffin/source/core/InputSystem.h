#pragma once
#ifndef GRIFFIN_INPUTSYSTEM_H_
#define GRIFFIN_INPUTSYSTEM_H_

#include <core/CoreSystem.h>
#include <vector>
#include <bitset>
#include <SDL_events.h>
#include <memory>
#include <utility/container/concurrent_queue.h>
#include <utility/enum.h>
#include <utility/container/handle_map.h>
#include <utility/concurrency.h>
#include <utility/memory_reserve.h>


using std::vector;
using std::bitset;

namespace griffin {
	namespace core {

		// Forward declarations

		class InputSystem;
		class InputContext;
		

		// Typedefs/Enums

		typedef std::shared_ptr<InputSystem> InputSystemPtr;
		typedef std::weak_ptr<InputSystem>   InputSystemWeakPtr;

		// Function declarations

		extern void setInputSystemPtr(InputSystemPtr inputPtr);


		// Type declarations

		/**
		* Input Cursor types
		*/
		MakeEnum(InputMouseCursors, uint8_t,
				 (Cursor_Arrow)
				 (Cursor_Hand)
				 (Cursor_Wait)
				 (Cursor_IBeam)
				 (Cursor_Crosshair)
				 , _T);

		/**
		* Input Event types
		*/
		MakeEnum(InputEventType, uint8_t,
				 (Keyboard)
				 (Mouse)
				 (Joystick)
				 (GameController)
				 (Touch)
				 (Text)
				 , _T);


		/**
		* Input Event
		*/
		struct InputEvent {
			InputEventType	type;
			int64_t			timeStampCounts;
			SDL_Event		evt;
		};


		/**
		* Input System
		*/
		class InputSystem : public CoreSystem {
		public:
			explicit InputSystem() :
				m_eventsQueue(RESERVE_INPUTSYSTEM_EVENTQUEUE)
			{
				m_popEvents.reserve(RESERVE_INPUTSYSTEM_POPQUEUE);
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
			* Executed on the update/render thread
			*/
			void update(const UpdateInfo& ui);

			/**
			* Executed on the input/GUI thread
			*/
			bool handleEvent(const SDL_Event& event);

			/**
			* Thread-safe blocking API to create a context and get back its handle
			*/
			Id_T createContext(uint16_t optionsMask) {
				auto f = tss_([optionsMask](ThreadSafeState& tss_) {
					return tss_.m_inputContexts.emplace(optionsMask);
				});
				return f;
			}

			/**
			* Text editing mode
			*/
			void startTextInput();
			void stopTextInput();
			bool isTextInputActive() const;

			/**
			* Mouse movement mode
			*/
			void startRelativeMouseMode();
			void stopRelativeMouseMode();
			bool isRelativeMouseModeActive() const;

		private:
			concurrent_queue<InputEvent>	m_eventsQueue;		//<! push on input thread, pop on update thread
			vector<InputEvent>				m_popEvents;		//<! pop events from the queue into this buffer

			struct ThreadSafeState {
				handle_map<InputContext>	m_inputContexts;	//<! collection of input contexts
				
				ThreadSafeState() : m_inputContexts(0, RESERVE_INPUT_CONTEXTS) {}
			};
			monitor<ThreadSafeState>		tss_;

			SDL_Cursor *					m_cursors[InputMouseCursorsCount]; //<! table of mouse cursors
			vector<SDL_Joystick*>			m_joysticks;		//<! list of opened joysticks
		};


		/**
		* Input Mapping types
		*/
		MakeEnum(InputMappingType, uint8_t,
				 (Action)	//<! Actions are single-time events
				 (State)	//<! States are on/off
				 (Axis)		//<! Axis are ranges of motion normalized to [-1,1]
				 , _T);

		/**
		* Input Mapping Binding types
		*/
		MakeEnum(InputMappingBindingType, uint8_t,
				 (Bind_Down)	//<! Bind to DOWN event of keyboard key, mouse or joystick button
				 (Bind_Up)		//<! Bind to UP event of keyboard key, mouse or joystick button
				 (Bind_Toggle)	//<! Axis are ranges of motion normalized to [-1,1]
				 , _T);

		/**
		* Input Mapping Axis Curve types
		*/
		MakeEnum(InputMappingAxisCurveType, uint8_t,
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
			InputMappingType			type = Action_T;
			InputMappingBindingType		bindIn = Bind_Down_T;	//<! event to start the action or state
			InputMappingBindingType		bindOut = Bind_Up_T;	//<! event to end the state
			InputMappingAxisCurveType	curve = Curve_SCurve_T;	//<! curve type of axis
			uint8_t						deadzone = 0;			//<! deadzone for axis
			uint8_t						curvature = 0;			//<! curvature of axis
			uint8_t						saturationX = 100;		//<! saturation of the x axis
			uint8_t						saturationY = 100;		//<! saturation of the y axis
			uint8_t						invert = 0;				//<! 0=false, 1=true axis is inverted
			uint8_t						slider = 0;				//<! 0=false, 1=true axis is a slider
			char						name[32];				//<! display name of the mapping
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
				 , NIL);

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
				m_options{ optionsMask }
			{
				m_inputMappings.reserve(RESERVE_INPUTCONTEXT_MAPPINGS);
			}

			InputContext(InputContext&& _c) _NOEXCEPT :
				m_options{ _c.m_options },
				m_inputMappings(std::move(_c.m_inputMappings))
			{
				_c.m_options = 0;
			}

			//InputContext(const InputContext&) = delete; // I want to delete this and force move semantics only, http://stackoverflow.com/questions/12251368/type-requirements-for-stdvectortype

			InputContext& operator=(InputContext&& _c)
			{
				if (this != &_c) {
					m_options = _c.m_options;
					_c.m_options = 0;
					m_inputMappings = std::move(_c.m_inputMappings);
				}
				return *this;
			}

		private:
			bitset<InputContextOptionsCount> m_options = {};	//<! all input context options
			//uint8_t					m_cursorIndex;			//<! lookup into input system's cursor table
			vector<InputMapping>	m_inputMappings = {};		//<! stores input mapping to actions, states, axes

		};

		
		/**
		*
		*/
		struct MappedInput {
			InputMappingType	type = Action_T;
			double				totalMs;
			int64_t				startCounts;
			int32_t				totalCounts;
			int32_t				startFrame;
			int32_t				totalFrames;
			InputMapping *		p_inputMapping;
		};
	}
}

#endif