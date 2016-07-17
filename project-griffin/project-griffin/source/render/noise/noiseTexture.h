#pragma once
#ifndef GRIFFIN_NOISE_TEXTURE_H_
#define GRIFFIN_NOISE_TEXTURE_H_

#include <resource/ResourceTypedefs.h>

namespace griffin {
	namespace render {
		namespace noise {
			resource::ResourcePtr createTestNoiseTexture();
		}
	}
}

#endif