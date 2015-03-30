#include <cstdio>
#include <memory>
#include <string>
#include <sstream>
#include <gl/glew.h>
//#include <gl/glcorearb.h>
#include "main.h"
#include "Timer.h"
#include <SOIL.h>
#include <entity/components.h>
#include <render/Render.h>
#include <utility/profile/Profile.h>
#include "platform.h"
#include "FixedTimestep.h"
#include <core/InputSystem.h>
#include <application/Application.h>

#define PROGRAM_NAME "Project Griffin"

using namespace griffin;
using std::unique_ptr;
using std::vector;
using std::string;

void test_resource_loader(); // test

int main(int argc, char *argv[])
{
	Timer::initHighPerfTimer();

	try {
		SDLApplication app;
		app.initWindow(PROGRAM_NAME);
		app.initOpenGL();
		auto application = make_application();
		
		render::initRenderData(app.getPrimaryWindow().width, app.getPrimaryWindow().height);

		test_reflection(); // TEMP

		bool done = false;
		int32_t frame = 0;

		// move input system into application
		core::InputSystem inputSystem;
		vector<core::CoreSystem*> systemUpdates = { &inputSystem }; // order of system execution for update

		SDL_GL_MakeCurrent(nullptr, NULL); // make no gl context current on the input thread

		// move this to Game class instead of having a lambda
		auto gameProcess = [&](){
			SDL_GL_MakeCurrent(app.getPrimaryWindow().window, app.getPrimaryWindow().glContext); // gl context made current on the main loop thread
			Timer timer;
			
			FixedTimestep update(1000.0 / 30.0, Timer::countsPerMs(),
				[&](const int64_t virtualTime, const int64_t gameTime, const int64_t deltaCounts,
					const double deltaMs, const double gameSpeed)
			{
				core::CoreSystem::UpdateInfo ui = { virtualTime, gameTime, deltaCounts, deltaMs, gameSpeed, frame };

				/*SDL_Log("Update virtualTime=%lu: gameTime=%ld: deltaCounts=%ld: countsPerMs=%ld\n",
						virtualTime, gameTime, deltaCounts, Timer::timerFreq() / 1000);*/

				// call all systems in priority order
				for (auto& s : systemUpdates) {
					s->update(ui);
				}
				// if all systems operate on 1(+) frame-old-data, can all systems be run in parallel?
				// should this simple vector become a task flow graph?
				// example systems:
				//	InputSystem
				//	AISystem
				//	ResourcePredictionSystem
				//	PhysicsSystem
				//	CollisionSystem
				//	etc.
			});

			int64_t realTime = timer.start();

			for (frame = 0; !done; ++frame) {
				PROFILE_BLOCK("main loop", frame, 2);

				int64_t countsPassed = timer.queryCountsPassed();
				realTime = timer.stopCounts();

				double interpolation = update.tick(realTime, countsPassed, 1.0);
				
				//SDL_Delay(1000);
				/*SDL_Log("Render realTime=%lu: interpolation=%0.3f: threadIdHash=%lu\n",
						realTime, interpolation, std::this_thread::get_id().hash());*/

				render::renderFrame(interpolation);

				SDL_GL_SwapWindow(app.getPrimaryWindow().window);
				platform::yieldThread();
			}
		};
		
		auto gameThread = std::async(std::launch::async, gameProcess);

		while (!done) {
			PROFILE_BLOCK("input loop", frame, 1);

			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				// send to the input system to handle the event
				bool handled = inputSystem.handleEvent(event);
				
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
			
			platform::yieldThread();
		}

	} catch (std::exception& e) {
		platform::showErrorBox(e.what(), "Error");
	}
	
	return 0;
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

	int width = 1280;
	int height = 720;

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
	glEnable(GL_MULTISAMPLE);

	// Enable back face culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	// Enable depth testing
	glEnable(GL_DEPTH_TEST);

	// Set the clear color to black and clear the screen
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
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