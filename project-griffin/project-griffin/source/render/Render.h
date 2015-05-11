#pragma once
#ifndef GRIFFIN_RENDER_H_
#define GRIFFIN_RENDER_H_

#include <string>
#include <memory>

namespace griffin {
	// Forward Declarations
	namespace resource { class ResourceLoader; }

	namespace render {

		using std::weak_ptr;
		using std::wstring;

		// Variables
		extern weak_ptr<resource::ResourceLoader> g_loaderPtr;

		// Functions
		void initRenderData(int viewportWidth, int viewportHeight);
		void renderFrame(double interpolation);

	}
}

#endif