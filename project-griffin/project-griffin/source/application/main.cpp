#include <cstdio>
#include <memory>
#include <string>
#include <sstream>
#include <gl/glew.h>
//#include <gl/glcorearb.h>
#include <SOIL.h>
#include "main.h"
#include <application/Timer.h>
#include <application/platform.h>
#include <application/UpdateInfo.h>
#include <application/FixedTimestep.h>
#include <application/Engine.h>
#include <game/Game.h>
#include <input/InputSystem.h>
#include <render/Render.h>
#include <resource/ResourceLoader.h>
#include <utility/profile/Profile.h>

#define PROGRAM_NAME "Project Griffin"

using namespace griffin;
using std::unique_ptr;
using std::vector;
using std::string;


int main(int argc, char *argv[])
{
	Timer::initHighPerfTimer();

	try {
		SDLApplication app;
		app.initWindow(PROGRAM_NAME);
		app.initOpenGL();

		auto engine = make_engine(app);
		auto game = make_game(engine, app);

		bool done = false;
		uint64_t frame = 0;

		SDL_GL_MakeCurrent(nullptr, NULL); // make no gl context current on the input thread

		/**
		* Game Update-Render Thread, runs the main rendering frame loop and the inner
		* fixed-timestep game update loop
		*/
		// move this a Game class instead of having a lambda??
		auto gameProcess = [&](){
			SDL_GL_MakeCurrent(app.getPrimaryWindow().window, app.getPrimaryWindow().glContext); // gl context made current on the main loop thread
			Timer timer;
			
			FixedTimestep update(1000.0f / 30.0f, Timer::countsPerMs(),
				[&](const int64_t virtualTime, const int64_t gameTime, const int64_t deltaCounts,
					const float deltaMs, const float gameSpeed)
			{
				PROFILE_BLOCK("update loop", frame, ThreadAffinity::Thread_Update);

				UpdateInfo ui = { virtualTime, gameTime, deltaCounts, deltaMs, gameSpeed, frame };

				/*SDL_Log("Update virtualTime=%lu: gameTime=%ld: deltaCounts=%ld: countsPerMs=%ld\n",
						virtualTime, gameTime, deltaCounts, Timer::timerFreq() / 1000);*/

				engine.threadPool->executeFixedThreadTasks(ThreadAffinity::Thread_Update);

				engineUpdateFrameTick(engine, ui);
			});

			int64_t realTime = timer.start();

			for (frame = 0; !done; ++frame) {
				PROFILE_BLOCK("render loop", frame, ThreadAffinity::Thread_OpenGL_Render);

				int64_t countsPassed = timer.queryCountsPassed();
				realTime = timer.stopCounts();

				float interpolation = update.tick(realTime, countsPassed, 1.0);
				
				//SDL_Delay(1000);
				/*SDL_Log("Render realTime=%lu: interpolation=%0.3f: threadIdHash=%lu\n",
						realTime, interpolation, std::this_thread::get_id().hash());*/
				
				engine.threadPool->executeFixedThreadTasks(ThreadAffinity::Thread_OpenGL_Render);
				
				engineRenderFrameTick(engine, interpolation);

				SDL_GL_SwapWindow(app.getPrimaryWindow().window);
				platform::yieldThread();
			}

			destroy_game(game);
			destroy_engine(engine); // delete the engine on the GL thread
		};
		
		// This actually starts the update-render thread. OpenGL context is transferred to this
		// thread after OpenGL is initialized on the OS thread. This thread joins in SDL_QUIT.
		auto gameThread = std::async(std::launch::async, gameProcess);

		/**
		* OS-Input Thread, runs the platform message loop
		*/
		while (!done) {
			PROFILE_BLOCK("input loop", frame, ThreadAffinity::Thread_OS_Input);

			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				// send to the input system to handle the event
				bool handled = engine.inputSystem->handleEvent(event);
				
				if (!handled) {
					switch (event.type) {
						case SDL_QUIT: {
							done = true; // input/GUI and game threads read this to exit
							gameThread.wait(); // waiting on the future forces the game thread to join
							break;
						}

						case SDL_WINDOWEVENT:
						case SDL_SYSWMEVENT:
							break;

						default: {
							SDL_Log("event type=%d\n", event.type);
						}
					}
				}
			}
			
			// run tasks with Thread_OS_Input thread affinity
			if (engine.threadPool) {
				engine.threadPool->executeFixedThreadTasks(ThreadAffinity::Thread_OS_Input);
			}
			
			// run thread_pool deferred task check (when_any, when_all)
			//   engine.taskPool.checkDeferredTasks();

			platform::yieldThread();
		}

	} catch (std::exception& e) {
		platform::showErrorBox(e.what(), "Error");
	}
	
	return 0;
}


SDLApplication::SDLApplication()
{
	systemInfo.cpuCount = SDL_GetCPUCount();
	systemInfo.systemRAM = SDL_GetSystemRAM();
}


void SDLApplication::initWindow(const char* appName)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		throw std::runtime_error(SDL_GetError());
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

	// Turn on antialiasing
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

	int width = 1600;
	int height = 900;

	auto window = SDL_CreateWindow(
		appName,
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		width, height,
		SDL_WINDOW_OPENGL);
		//0, 0,
		//SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP);

	if (window == nullptr) {
		throw std::runtime_error(SDL_GetError());
	}

	auto glContext = SDL_GL_CreateContext(window);

	if (glContext == nullptr) {
		throw std::runtime_error(SDL_GetError());
	}

	windowData.resize(1);
	windowData[0].window = window;
	windowData[0].glContext = glContext;
	windowData[0].width = width;
	windowData[0].height = height;
	windowData[0].wmInfo = platform::getWindowInfo(window);

	platform::setWindowIcon(&windowData[0]);

	SDL_DisableScreenSaver();
}


void SDLApplication::initOpenGL()
{
	// Initialize GLEW
	glewExperimental = true; // Needed in core profile
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		SDL_Log("Failed to initialize GLEW\n");
		throw std::runtime_error((const char *)glewGetErrorString(err));
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

	// Enable multisampling
	//glEnable(GL_MULTISAMPLE);

	// Enable back face culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	// Enable depth testing
	glEnable(GL_DEPTH_TEST);

	// Set the clear color to black and clear the screen
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SDL_GL_SwapWindow(getPrimaryWindow().window);
}


SDLApplication::~SDLApplication()
{
	for (auto& wd : windowData) {
		SDL_GL_DeleteContext(wd.glContext);
		SDL_DestroyWindow(wd.window);
	}

	SDL_Quit();
}
