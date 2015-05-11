#include <api/InputSystemApi.h>
#include <core/InputSystem.h>
#include <SDL_log.h>

griffin::core::InputSystemPtr g_inputPtr = nullptr;

namespace griffin {
	namespace core {

		void setInputSystemPtr(InputSystemPtr inputPtr)
		{
			g_inputPtr = inputPtr;
		}

	}
}

#ifdef __cplusplus
extern "C" {
#endif

	using namespace griffin;
	using namespace griffin::core;

	GRIFFIN_EXPORT
	uint64_t griffin_input_createContext(uint16_t optionsMask, uint8_t priority, const char name[32], bool makeActive)
	{
		auto& input = *g_inputPtr;

		auto id = g_inputPtr->createContext(optionsMask, priority, makeActive);
		auto& context = input.getContext(id);
		context.contextId = id;
		strncpy_s(context.name, name, 32);

		return id.value;
	}


	GRIFFIN_EXPORT
	bool griffin_input_setContextActive(uint64_t context, bool active)
	{
		auto& input = *g_inputPtr;

		Id_T contextId;
		contextId.value = context;

		return input.setContextActive(contextId, active);
	}


	GRIFFIN_EXPORT
	uint64_t griffin_input_createInputMapping(const char name[32], uint64_t context)
	{
		assert(sizeof(griffin_InputMapping) == sizeof(InputMapping) && "InputMapping struct out of sync with API");

		auto& input = *g_inputPtr;

		Id_T contextId;
		contextId.value = context;
		
		auto id = input.createInputMapping(InputMapping{});
		auto& mapping = input.getInputMapping(id);
		mapping.mappingId = id;
		mapping.contextId = contextId;
		strncpy_s(mapping.name, name, 32);

		input.getContext(contextId).inputMappings.push_back(id);

		return id.value;
	}


	GRIFFIN_EXPORT
	griffin_InputMapping* griffin_input_getInputMapping(uint64_t mapping)
	{
		auto& input = *g_inputPtr;

		Id_T mappingId;
		mappingId.value = mapping;

		return reinterpret_cast<griffin_InputMapping*>(&input.getInputMapping(mappingId));
	}

	GRIFFIN_EXPORT
	uint64_t griffin_input_registerCallback(int priority, Callback_T callbackFunc)
	{
		assert(sizeof(griffin_MappedAction) == sizeof(MappedAction) && "MappedAction struct out of sync with API");
		assert(sizeof(griffin_MappedState) == sizeof(MappedState) && "MappedState struct out of sync with API");
		assert(sizeof(griffin_MappedAxis) == sizeof(MappedAxis) && "MappedAxis struct out of sync with API");
		assert(sizeof(griffin_AxisMotion) == sizeof(AxisMotion) && "AxisMotion struct out of sync with API");
		
		auto& input = *g_inputPtr;

		Id_T id = input.registerCallback(priority, [callbackFunc](FrameMappedInput& mappedInput) {
			griffin_FrameMappedInput mi{};
			mi.actions = reinterpret_cast<griffin_MappedAction*>(mappedInput.actions.data());
			mi.actionsSize = static_cast<int16_t>(mappedInput.actions.size());
			mi.states = reinterpret_cast<griffin_MappedState*>(mappedInput.states.data());
			mi.statesSize = static_cast<int16_t>(mappedInput.states.size());
			mi.axes = reinterpret_cast<griffin_MappedAxis*>(mappedInput.axes.data());
			mi.axesSize = static_cast<int16_t>(mappedInput.axes.size());
			mi.axisMotion = reinterpret_cast<griffin_AxisMotion*>(mappedInput.motion.data());
			mi.axisMotionSize = static_cast<int16_t>(mappedInput.motion.size());
			mi.textInput = mappedInput.textInput.c_str();
			mi.textInputLength = static_cast<int16_t>(mappedInput.textInput.length());

			callbackFunc(&mi);
		});
		return id.value;
	}

	GRIFFIN_EXPORT
	bool griffin_input_removeCallback(uint64_t callback)
	{
		return false;
	}

	GRIFFIN_EXPORT
	void griffin_input_setRelativeMouseMode(bool relative)
	{
		auto& input = *g_inputPtr;

		if (relative) {
			input.startRelativeMouseMode();
		}
		else {
			input.stopRelativeMouseMode();
		}
	}

	GRIFFIN_EXPORT
	bool griffin_input_relativeMouseModeActive()
	{
		return g_inputPtr->relativeMouseModeActive();
	}

#ifdef __cplusplus
}
#endif