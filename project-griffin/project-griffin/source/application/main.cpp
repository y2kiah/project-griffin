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
#include "platform.h"
#include "FixedTimestep.h"
#include <core/InputSystem.h>


#define PROGRAM_NAME "Project Griffin"

using namespace griffin;
using std::unique_ptr;
using std::vector;
using std::string;

int main(int argc, char *argv[])
{
	Timer::initHighPerfTimer();

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
		int32_t frame = 0;

		core::InputSystem inputSystem;
		vector<core::CoreSystem*> systemUpdates = { &inputSystem }; // order of system execution for update

		SDL_GL_MakeCurrent(nullptr, NULL); // make no gl context current on the input thread

		// move this to Game class instead of having a lambda
		auto gameProcess = [&](){
			SDL_GL_MakeCurrent(app.getPrimaryWindow(), app.getGLContext()); // gl context made current on the main loop thread
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

				renderFrame(interpolation);

				SDL_GL_SwapWindow(app.getPrimaryWindow());
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
