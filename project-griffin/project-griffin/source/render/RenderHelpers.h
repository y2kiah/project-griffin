#pragma once
#ifndef GRIFFIN_RENDER_HELPERS_H_
#define GRIFFIN_RENDER_HELPERS_H_

#include <cstdint>

namespace griffin {
	namespace render {

		// Drawing shapes and volumes
		void drawFullscreenQuad();
		void drawCube();
		void drawPixelPerfectQuad(float leftPx, float topPx, uint32_t widthPx, uint32_t heightPx);
		void drawScaledQuad(float left, float top, float width, float height);

		// Lighting
		float getLightVolumeRadius(float Imax, float Kc, float Kl, float Kq);

	}
}


#endif