#pragma once
#ifndef GRIFFIN_MAIN_H_
#define GRIFFIN_MAIN_H_

#include <vector>
#include <memory>
#include <SDL.h>

using std::vector;
using std::shared_ptr;
struct SDL_SysWMinfo;


struct DisplayData {
	SDL_Rect					bounds;			//<! bounds of the display
	vector<SDL_DisplayMode>		displayModes;	//<! list of display modes
};

struct WindowData {
	SDL_Window *				window;			//<! SDL window handle
	SDL_GLContext				glContext;		//<! OpenGL context handle
	uint32_t					width;			//<! width px of the window
	uint32_t					height;			//<! height px of the window
	shared_ptr<SDL_SysWMinfo>	wmInfo;			//<! system-dependent window information
};

struct SystemInfo {
	uint8_t						cpuCount;		//<! number of logical CPU cores
	uint32_t					systemRAM;		//<! amount of system RAM in MB
};

typedef vector<DisplayData>	DisplayDataList;
typedef vector<WindowData>	WindowDataList;


class SDLApplication {
public:
	explicit SDLApplication();
	~SDLApplication();

	void initWindow(const char* appName);
	void initOpenGL();

	const WindowData& getPrimaryWindow() const
	{
		return windowData[0];
	}

	const SystemInfo& getSystemInfo() const
	{
		return systemInfo;
	}

private:
	DisplayDataList	displayData;
	WindowDataList	windowData;
	SystemInfo		systemInfo;
};

#endif