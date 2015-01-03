#pragma once
#ifndef MAIN_H
#define MAIN_H

#include <vector>
#include <SDL.h>

using std::vector;

struct DisplayData {
	SDL_Rect bounds;	// bounds of the window
	vector<SDL_DisplayMode> displayModes;	// list of display modes
};

struct WindowData {
	SDL_Window *window;			// SDL window handle
	SDL_GLContext glContext;	// OpenGL context handle
};

typedef vector<DisplayData>	DisplayDataList;
typedef vector<WindowData>	WindowDataList;


// MOVE ALL OF THE IMPLEMENTATIONS OUT OF THE HEADER
class SDLApplication {
private:
	DisplayDataList	displayData;
	WindowDataList	windowData;

public:
	SDL_Window* getPrimaryWindow() const
	{
		return windowData[0].window;
	}
	const SDL_GLContext& getGLContext() const
	{
		return windowData[0].glContext;
	}

	explicit SDLApplication(const char* appName)
	{
		if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
			throw(std::runtime_error(SDL_GetError()));
		}

		// enable logging
		SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);

		// get number of displays
		int displayNum = SDL_GetNumVideoDisplays();
		displayData.resize(displayNum);

		// get all display modes for each display
		for (int d = 0; d < displayNum; ++d) {
			DisplayData &dd = displayData[d];
			SDL_GetDisplayBounds(d, &(dd.bounds));

			int modesNum = SDL_GetNumDisplayModes(d);
			dd.displayModes.resize(modesNum);
			for (int m = 0; m < modesNum; ++m) {
				int result = SDL_GetDisplayMode(d, m, &dd.displayModes[m]);
			}
		}

		// Request opengl 4.4 context
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		//WGL_NV_gpu_affinity
		//AMD_gpu_association

		// Turn on double buffering with a 24bit Z buffer.
		// You may need to change this to 16 or 32 for your system
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

		windowData.resize(1);
		windowData[0].window = SDL_CreateWindow(
			appName,
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			640, 480,
			SDL_WINDOW_OPENGL);
			//0, 0,
			//SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP);

		if (getPrimaryWindow() == nullptr) {
			throw(std::runtime_error(SDL_GetError()));
		}

		windowData[0].glContext = SDL_GL_CreateContext(getPrimaryWindow());
		if (windowData[0].glContext == nullptr) {
			throw(std::runtime_error(SDL_GetError()));
		}
	}

	~SDLApplication()
	{
		for (auto& wd : windowData) {
			SDL_GL_DeleteContext(wd.glContext);
			SDL_DestroyWindow(wd.window);
		}

		SDL_Quit();
	}
};

#endif