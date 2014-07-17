#include <cstdio>
#include <memory>
#include <string>
#include <sstream>
#include <GL/glew.h>
//#include <GL/glcorearb.h>
#include "main.h"
#include "Timer.h"
#include <SOIL.h>
#include <component/components.h>
#include <render/Render.h>
#include <utility/profile/Profile.h>

#include <utility/concurrency.h>

#define PROGRAM_NAME "Project Griffin"

using std::unique_ptr;
using std::vector;
using std::string;

int main(int argc, char *argv[])
{
	griffin::Timer::initHighPerfTimer();

	try {
		SDLApplication app(PROGRAM_NAME);

		// Initialize GLEW
		glewExperimental = true; // Needed in core profile
		GLenum err = glewInit();
		if (err != GLEW_OK) {
			SDL_Log("Failed to initialize GLEW\n");
			throw(std::runtime_error((const char *)glewGetErrorString(err)));
		}

		SDL_Log("OpenGL Information:\n  Vendor: %s\n  Renderer: %s\n  Version: %s\n  Shading Language Version: %s\n",
			glGetString(GL_VENDOR),
			glGetString(GL_RENDERER),
			glGetString(GL_VERSION),
			glGetString(GL_SHADING_LANGUAGE_VERSION)
		);
		std::ostringstream glExtensionsSS;
		//PFNGLGETSTRINGIPROC glGetStringi = (PFNGLGETSTRINGIPROC)SDL_GL_GetProcAddress("glGetStringi");

		// Use the post-3.0 method for querying extensions
		int numExtensions = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
		for (int i = 0; i < numExtensions; ++i) {
			glExtensionsSS << (const char*)glGetStringi(GL_EXTENSIONS, i) << " ";
		}
		string glExtensions(glExtensionsSS.str());
		SDL_Log("OpenGL Extensions: %s\n", glExtensions.c_str());
		
		// give the extensions string to the SOIL library
		SOIL_set_gl_extensions_string(glExtensions.c_str());

		// This makes our buffer swap syncronized with the monitor's vertical refresh
		SDL_GL_SetSwapInterval(1);

		glClearColor(0.0, 0.0, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		SDL_GL_SwapWindow(app.getPrimaryWindow());

		// load an image file directly as a new OpenGL texture
		GLuint tex_2d = SOIL_load_OGL_texture(
			"vendor/soil/img_test.png",
			SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID,
			SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_DDS_LOAD_DIRECT | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT);

		SDL_Log("tex: %d\n", tex_2d);
		// check for an error during the load process
		if (tex_2d == 0) {
			SDL_Log("SOIL loading error: %s\n", SOIL_last_result());
		}
		
		test_reflection();
		initRenderData();

		bool done = false;


		SDL_GL_MakeCurrent(nullptr, NULL); // make no gl context current on the input thread

		auto mainProcess = [&](){
			SDL_GL_MakeCurrent(app.getPrimaryWindow(), app.getGLContext()); // gl context made current on the main loop thread
			for (int32_t frame = 0; !done; ++frame) {
				SDL_GL_SwapWindow(app.getPrimaryWindow());
				renderFrame();
			}
		};

		auto mainThread = std::async(mainProcess, std::launch::async);

		for (;;) {
			//PROFILE_BLOCK("main loop", frame)

			SDL_Event event;
			if (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT) {
					done = true; // main thread will read this and exit
					break;
				}
				//	TranslateMessage(&msg);
				//	DispatchMessage(&msg);
			} else {
				//} else if (win32.isActive()) {
				//	if (win32.isExiting()) { PostMessage(win32.hWnd, WM_CLOSE, 0, 0); }

				//	app->processFrame();

				//} else {
				//	WaitMessage();	// avoid 100% CPU when inactive
			}
		}

	} catch (std::exception& e) {
		showErrorBox(e.what(), "Error");
	}

	return 0;
}

#ifdef _WIN32

#include <Windows.h>

void showErrorBox(const char *text, const char *caption)
{
	MessageBoxA(NULL, text, caption, MB_OK | MB_ICONERROR | MB_TOPMOST);
}

#else

void showErrorBox(const char *text, const char *caption)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, caption, text, nullptr);
}

#endif