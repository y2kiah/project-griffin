#include "../platform.h"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "Windows.h"

namespace griffin {
	namespace platform {

		void yieldThread()
		{
			SwitchToThread();
		}

		void showErrorBox(const char *text, const char *caption)
		{
			MessageBoxA(NULL, text, caption, MB_OK | MB_ICONERROR | MB_TOPMOST);
		}

	}
}

#else

namespace griffin {
	namespace platform {

		void yieldThread()
		{
			std::this_thread::yield(); // temporary look for best posix yield method
		}

		void showErrorBox(const char *text, const char *caption)
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, caption, text, nullptr);
		}

	}
}

#endif