#pragma once
#ifndef GRIFFIN_RENDER_
#define GRIFFIN_RENDER_

#include <string>
#include <memory>

namespace griffin {
	// Forward Declarations
	namespace resource { class ResourceLoader; }

	namespace render {

		using std::weak_ptr;
		using std::string;

		// Variables
		extern weak_ptr<resource::ResourceLoader> g_loaderPtr;

		// Functions
		void initRenderData();
		void renderFrame(double interpolation);
		unsigned int loadShaders(string vertexFilePath, string fragmentFilePath);

	}
}

#endif