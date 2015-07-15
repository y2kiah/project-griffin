#include "application/platform.h"
#include "application/main.h"
#include <SDL_filesystem.h>
#include <SDL_syswm.h>
#include "../resource.h"


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

		shared_ptr<SDL_SysWMinfo> getWindowInfo(SDL_Window* window)
		{
			auto info = std::make_shared<SDL_SysWMinfo>();
			SDL_VERSION(&info->version);
			if (SDL_GetWindowWMInfo(window, info.get()) != SDL_TRUE) {
				throw std::runtime_error(SDL_GetError());
			}
			return info;
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

		void setWindowIcon(const WindowData *windowData)
		{
			HWND hWnd = windowData->wmInfo->info.win.window;
			HINSTANCE handle = GetModuleHandle(nullptr);

			HICON icon = LoadIcon(handle, MAKEINTRESOURCE(IDI_ICON1));
			if (icon != nullptr) {
				SetClassLongPtr(hWnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(icon));
				SetClassLongPtr(hWnd, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(icon));
			}
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

		void setWindowIcon(const WindowData *windowData)
		{
			assert("use SDL here");
			//	SDL_Surface* icon = IMG_Load(const char *file);
			//	SDL_SetWindowIcon(window, icon);
			//	SDL_FreeSurface(icon);
		}

	}
}

#endif
