#pragma once
#ifndef GRIFFIN_RENDER_HELPERS_H_
#define GRIFFIN_RENDER_HELPERS_H_

#include <cstdint>

namespace griffin {
	namespace render {

		void setViewportDimensions(uint32_t left, uint32_t top, uint32_t width, uint32_t height);

		void drawFullscreenQuad();
		void drawPixelPerfectQuad(float leftPx, float topPx, uint32_t widthPx, uint32_t heightPx);
		void drawPixelPerfectQuad(float left, float top, uint32_t widthPx, uint32_t heightPx);
		void drawScaledQuad(float left, float top, float width, float height);

	}
}


#endif