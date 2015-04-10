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

		class InputSystem;
		class InputContext;

		typedef std::shared_ptr<InputSystem> InputSystemPtr;

		/**
		* Input Mapping types
		*/
		MakeEnum(InputMappingType, uint8_t,
				 (Action)	//<! Actions are single-time events
				 (State)	//<! States are on/off
				 (Axis)		//<! Axis are ranges of motion normalized to [-1,1]
				 , _T);

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
		* Input mappings map raw input events or position data to Actions, States or Ranges
		*	Actions: Single-time events, not affected by key repeat, mapped to a single input event
		*			 e.g. {keydown-G} or {keyup-G}.
		*	States:  Binary on/off flag mapped to two input events e.g. {on:keydown-G, off:keyup-G}
		*			 or {on:keydown-G, off:keydown-G}. The first example would be typical of WASD
		*			 type movement state, the second example would behave like a toggle.
		*	Axis:	 Uses position information of joysticks, throttles, rudder pedals, head
		*			 tracking gear, even mouse movement if desired.
		*/
		struct InputMapping {
			InputMappingType	type = Action_T;

		};


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

			//InputContext(InputContext&& _c) {}
			//InputContext(const InputContext&) = delete; // I want to delete this and force move semantics only

		private:
			bitset<InputContextOptionsCount> m_options = {};	//<! all input context options
			//uint8_t					m_cursorIndex;			//<! lookup into input system's cursor table
			vector<InputMapping>	m_inputMappings = {};		//<! stores input mapping to actions, states, axes

		};

		
	}
}

#endif