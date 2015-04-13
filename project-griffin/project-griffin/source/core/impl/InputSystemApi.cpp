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
	uint64_t griffin_input_createContext() {
		auto t = g_inputPtr->createContext(0);
		return t.value;
	}

#ifdef __cplusplus
}
#endif