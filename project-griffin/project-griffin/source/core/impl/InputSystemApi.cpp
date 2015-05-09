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
		auto& input = *g_inputPtr;
		assert(sizeof(griffin_InputMapping) == sizeof(InputMapping) && "InputMapping struct out of sync with API");

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

#ifdef __cplusplus
}
#endif