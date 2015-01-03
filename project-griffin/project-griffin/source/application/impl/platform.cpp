#include "../platform.h"
#include <SDL_filesystem.h>

namespace griffin {
	namespace platform {

		std::string getPreferencesPath()
		{
			std::string s;
			auto c = SDL_GetPrefPath("Griffin", "Project Griffin");
			s.assign(c);
			SDL_free(c);
			return s;
		}

		std::wstring getPreferencesPathW()
		{
			std::wstring ws;

			auto s = getPreferencesPath();
			ws.assign(s.begin(), s.end());

			return ws;
		}

	}
}

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "Windows.h"
#undef min
#undef max

namespace griffin {
	namespace platform {

		std::string getCurrentWorkingDirectory()
		{
			char currentDir[MAX_PATH];
			GetCurrentDirectoryA(MAX_PATH, currentDir);

			return currentDir;
		}

		std::wstring getCurrentWorkingDirectoryW()
		{
			std::wstring ws;

			wchar_t currentDir[MAX_PATH];
			GetCurrentDirectory(MAX_PATH, currentDir);
			ws = currentDir;

			return ws;
		}

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

		std::string getCurrentWorkingDirectory()
		{
			std::string s;
			auto c = SDL_GetBasePath();
			s.assign(c);
			SDL_free(c);
			return s;
		}

		std::wstring getCurrentWorkingDirectoryW()
		{
			std::wstring ws;

			auto s = getCurrentWorkingDirectory();
			ws.assign(s.begin(), s.end());

			return ws;
		}

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
