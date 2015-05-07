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

	GRIFFIN_EXPORT
	uint64_t griffin_input_createContext(uint8_t priority, bool makeActive) {
		auto id = g_inputPtr->createContext(0, priority, makeActive);
		return id.value;
	}

	GRIFFIN_EXPORT
	uint64_t griffin_input_createInputMapping(const griffin_InputMapping* mapping, uint64_t context) {
		griffin::Id_T contextId;
		contextId.value = context;
		
		griffin::core::InputMapping inputMapping;
		memcpy_s(&inputMapping, sizeof(inputMapping), mapping, sizeof(griffin_InputMapping));

		auto& input = *g_inputPtr;
		auto id = input.createInputMapping(std::move(inputMapping));
		input.getContext(contextId).inputMappings.push_back(id);

		return id.value;
	}

#ifdef __cplusplus
}
#endif