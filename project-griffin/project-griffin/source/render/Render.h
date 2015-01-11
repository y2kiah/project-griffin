#pragma once
#ifndef GRIFFIN_RENDER_
#define GRIFFIN_RENDER_

#include <string>

namespace griffin {
	namespace render {

		void initRenderData();
		void renderFrame(double interpolation);
		unsigned int loadShaders(std::string vertexFilePath, std::string fragmentFilePath);

	}
}

#endif