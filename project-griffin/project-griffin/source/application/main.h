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
	int width, height;
};

typedef vector<DisplayData>	DisplayDataList;
typedef vector<WindowData>	WindowDataList;


class SDLApplication {
public:
	explicit SDLApplication() {}
	~SDLApplication();

	void initWindow(const char* appName);
	void initOpenGL();

	const WindowData& getPrimaryWindow() const
	{
		return windowData[0];
	}

private:
	DisplayDataList	displayData;
	WindowDataList	windowData;
};

#endif