#include "noiseTexture.h"
#include "noise.h"
#include <render/texture/Texture2D_GL.h>
#include <resource/ResourceLoader.h>

using namespace griffin;
using namespace griffin::render;

extern resource::ResourceLoaderWeakPtr g_resourceLoader; // defined in Render.cpp


resource::ResourcePtr noise::createTestNoiseTexture() {
	const int texSizeX = 64;
	const int texSizeY = 64;
	const float length = 1.0f;
	const int numChannels = 1;
	const int bufferSize = texSizeY * texSizeX * numChannels;

	uint8_t *buffer = new uint8_t[bufferSize];
	
	const float xStep = length / static_cast<float>(texSizeX);
	const float yStep = length / static_cast<float>(texSizeY);

	float v[2] = {};
	for (int y = 0; y < texSizeY; ++y) {
		for (int x = 0; x < texSizeX; ++x) {
			//float nResult = noise::multiFractal(2, v, 1, 2.0f, 0.3f, 0.6f);
			//float nResult = noise::simplexNoise2(x/length, y/length);
			//float nResult = noise::coherentNoise(2, v, CoherentNoiseType_Simplex);
			float nResult = noise::fBm(2, v, 8, 2.0f, 0.4f, 1.25f, MultiFractalOperation_Add_Abs);
			
			//nResult *= noise::simplexNoise2(x/32.0f, y/32.0f);

			//nResult = (nResult + 1.0f) * 0.35f + 0.2f; // normalize to [0,1] range
			//nResult = 1.0f - nResult;

			//			noise::setMultiFractalOperation(NOISE_MFO_ADD_ABS);
			//			float nResult2 = noise::fBm(2, v, 18, 2.0f, 0.5f, 0.6f);
			//			nResult2 *= nResult2;

			//			nResult *= nResult2;		

			int iResult = static_cast<int>(round(nResult * 255));  // convert to int
			uint8_t ucResult = (iResult > 255) ? 255 : ((iResult < 0) ? 0 : iResult); // convert to uint8_t

			int index = (y*texSizeX*numChannels) + (x*numChannels);
			for (int c = 0; c < numChannels; ++c) buffer[index + c] = ucResult;

			v[0] += xStep;
		}
		v[0] = 0;
		v[1] += yStep;
	}

	// create texture and add it to the materials cache
	Texture2D_GL tex;
	tex.createFromMemory(buffer, bufferSize, texSizeX, texSizeY, numChannels);

	resource::ResourcePtr resPtr = std::make_shared<resource::Resource_T>(std::move(tex), bufferSize);

	auto loader = g_resourceLoader.lock();
	auto texHandle = loader->addResourceToCache<Texture2D_GL>(resPtr, resource::CacheType::Cache_Materials, L"temp_noise");
	auto f = loader->getResource(texHandle);
	return f.get();
}