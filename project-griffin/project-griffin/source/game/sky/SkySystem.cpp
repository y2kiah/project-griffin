#include "SkySystem.h"
#include "atmosphere.h"
#include <render/Render.h>
#include <render/RenderResources.h>
#include <render/RenderHelpers.h>
#include <render/RenderTarget_GL.h>
#include <render/RenderTarget3D_GL.h>
#include <GL/glew.h>
#include <utility/Debug.h>

void precomputeAtmosphere(griffin::game::SkySystem& sky,
						  const griffin::render::RenderSystemPtr &renderSystemPtr);
void setLayer(unsigned int prog, int layer);


void griffin::game::SkySystem::updateFrameTick(Game* pGame, Engine& engine, const UpdateInfo& ui)
{
	// this function could implement changing sky conditions
}


void griffin::game::SkySystem::init(Game* pGame, const Engine& engine, const SDLApplication& app)
{
	using namespace render;
	using namespace resource;

	auto skyTex = loadTextureCubeMap(L"textures/skybox.dds", CacheType::Cache_Permanent, true, true);
	skyBoxCubeMap = engine.resourceLoader->getResource(skyTex).get();
	precomputeAtmosphere(*this, engine.renderSystem);
}


void precomputeAtmosphere(griffin::game::SkySystem& sky,
						  const griffin::render::RenderSystemPtr &renderSystemPtr)
{
	using namespace griffin::render;
	using griffin::resource::ResourcePtr;

	auto loader = g_resourceLoader.lock();
	if (!loader) {
		throw std::runtime_error("no resource loader");
	}


	// Create pre-calc render targets

	// T table
	RenderTarget_GL transmittanceTexture(RenderTarget_GL::RenderTargetType::TypeFloat16);
	uint32_t transmittanceUnit = 0;
	transmittanceTexture.init(TRANSMITTANCE_W, TRANSMITTANCE_H);
	transmittanceTexture.bind(RenderTarget_GL::RenderTargetTexture::Float16, transmittanceUnit);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// E table
	RenderTarget_GL irradianceTexture(RenderTarget_GL::RenderTargetType::TypeFloat16);
	uint32_t irradianceUnit = 1;
	irradianceTexture.init(SKY_W, SKY_H);
	irradianceTexture.bind(RenderTarget_GL::RenderTargetTexture::Float16, irradianceUnit);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// S table
	RenderTarget3D_GL inscatterTexture(RenderTarget3D_GL::RenderTarget3DType::TypeFloat16, 1);
	uint32_t inscatterUnit = 2;
	inscatterTexture.init(RES_MU_S * RES_NU, RES_MU, RES_R);
	inscatterTexture.bind(0, inscatterUnit);

	// deltaE table
	RenderTarget_GL deltaETexture(RenderTarget_GL::RenderTargetType::TypeFloat16);
	uint32_t deltaEUnit = 3;
	deltaETexture.init(SKY_W, SKY_H);
	deltaETexture.bind(RenderTarget_GL::RenderTargetTexture::Float16, deltaEUnit);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// deltaS table (Rayleigh part)
	// deltaS table (Mie part)
	RenderTarget3D_GL deltaSR_SMTexture(RenderTarget3D_GL::RenderTarget3DType::TypeFloat16, 2);
	uint32_t deltaSRUnit = 4;
	uint32_t deltaSMUnit = 5;
	deltaSR_SMTexture.init(RES_MU_S * RES_NU, RES_MU, RES_R);
	deltaSR_SMTexture.bind(0, deltaSRUnit);
	deltaSR_SMTexture.bind(1, deltaSMUnit);

	// deltaJ table
	RenderTarget3D_GL deltaJTexture(RenderTarget3D_GL::RenderTarget3DType::TypeFloat16, 1);
	uint32_t deltaJUnit = 6;
	deltaSR_SMTexture.init(RES_MU_S * RES_NU, RES_MU, RES_R);
	deltaSR_SMTexture.bind(0, deltaJUnit);


	// Load and compile shader programs

	auto trans = loadShaderProgram(L"shaders/atmosphere/transmittance.glsl", renderSystemPtr);
	auto irr1 = loadShaderProgram(L"shaders/atmosphere/irradiance1.glsl", renderSystemPtr);
	auto irrN = loadShaderProgram(L"shaders/atmosphere/irradianceN.glsl", renderSystemPtr);
	auto insc1 = loadShaderProgram(L"shaders/atmosphere/inscatter1.glsl", renderSystemPtr);
	auto inscN = loadShaderProgram(L"shaders/atmosphere/inscatterN.glsl", renderSystemPtr);
	auto inscS = loadShaderProgram(L"shaders/atmosphere/inscatterS.glsl", renderSystemPtr);
	auto cpIr = loadShaderProgram(L"shaders/atmosphere/copyIrradiance.glsl", renderSystemPtr);
	auto cpIn1 = loadShaderProgram(L"shaders/atmosphere/copyInscatter1.glsl", renderSystemPtr);
	auto cpInN = loadShaderProgram(L"shaders/atmosphere/copyInscatterN.glsl", renderSystemPtr);
	auto atms = loadShaderProgram(L"shaders/atmosphere/atmosphere.glsl", renderSystemPtr);

	sky.transmittanceProgram = loader->getResource(trans).get();
	sky.irradiance1Program = loader->getResource(irr1).get();
	sky.irradianceNProgram = loader->getResource(irrN).get();
	sky.inscatter1Program = loader->getResource(insc1).get();
	sky.inscatterNProgram = loader->getResource(inscN).get();
	sky.inscatterSProgram = loader->getResource(inscS).get();
	sky.copyIrradianceProgram = loader->getResource(cpIr).get();
	sky.copyInscatter1Program = loader->getResource(cpIn1).get();
	sky.copyInscatterNProgram = loader->getResource(cpInN).get();
	sky.atmosphereProgram = loader->getResource(atms).get();

	/*

	files.clear();
	files.push_back("Main.h");
	files.push_back("common.glsl");
	files.push_back("earth.glsl");
	drawProg = loadProgram(files);
	glUseProgram(drawProg);
	glUniform1i(glGetUniformLocation(drawProg, "reflectanceSampler"), reflectanceUnit);
	glUniform1i(glGetUniformLocation(drawProg, "transmittanceSampler"), transmittanceUnit);
	glUniform1i(glGetUniformLocation(drawProg, "irradianceSampler"), irradianceUnit);
	glUniform1i(glGetUniformLocation(drawProg, "inscatterSampler"), inscatterUnit);
	*/

	// Do precomputations

	// computes transmittance texture T (line 1 in algorithm 4.1)
	transmittanceTexture.start();
	{
		auto& prg = sky.transmittanceProgram.get()->getResource<ShaderProgram_GL>();
		prg.useProgram();
		drawFullscreenQuad();
	}
	transmittanceTexture.stop();

	// computes irradiance texture deltaE (line 2 in algorithm 4.1)
	deltaETexture.start();
	{
		auto& prg = sky.irradiance1Program.get()->getResource<ShaderProgram_GL>();
		prg.useProgram();
		auto transmittanceSamplerLoc = glGetUniformLocation(prg.getProgramId(), "transmittanceSampler");
		glUniform1i(transmittanceSamplerLoc, transmittanceUnit);

		drawFullscreenQuad();
	}
	deltaETexture.stop();

	// computes single scattering texture deltaS (line 3 in algorithm 4.1)
	// Rayleigh and Mie separated in deltaSR + deltaSM
	deltaSR_SMTexture.start();
	{
		auto& prg = sky.inscatter1Program.get()->getResource<ShaderProgram_GL>();
		prg.useProgram();
		glUniform1i(glGetUniformLocation(prg.getProgramId(), "transmittanceSampler"), transmittanceUnit);

		for (int layer = 0; layer < RES_R; ++layer) {
			setLayer(prg.getProgramId(), layer);
			drawFullscreenQuad();
		}
	}
	deltaSR_SMTexture.stop();

	// copies deltaE into irradiance texture E (line 4 in algorithm 4.1)
	irradianceTexture.start();
	{
		auto& prg = sky.copyIrradianceProgram.get()->getResource<ShaderProgram_GL>();
		prg.useProgram();
		glUniform1f(glGetUniformLocation(prg.getProgramId(), "k"), 0.0f);
		glUniform1i(glGetUniformLocation(prg.getProgramId(), "deltaESampler"), deltaEUnit);
		drawFullscreenQuad();
	}
	irradianceTexture.stop();

	// copies deltaS into inscatter texture S (line 5 in algorithm 4.1)
	inscatterTexture.start();
	{
		auto& prg = sky.copyInscatter1Program.get()->getResource<ShaderProgram_GL>();
		prg.useProgram();
		glUniform1i(glGetUniformLocation(prg.getProgramId(), "deltaSRSampler"), deltaSRUnit);
		glUniform1i(glGetUniformLocation(prg.getProgramId(), "deltaSMSampler"), deltaSMUnit);
		for (int layer = 0; layer < RES_R; ++layer) {
			setLayer(prg.getProgramId(), layer);
			drawFullscreenQuad();
		}
	}
	inscatterTexture.stop();

	// loop for each scattering order (line 6 in algorithm 4.1)
	for (int order = 2; order <= 4; ++order) {

		// computes deltaJ (line 7 in algorithm 4.1)
		deltaJTexture.start();
		{
			auto& prg = sky.inscatterSProgram.get()->getResource<ShaderProgram_GL>();
			prg.useProgram();
			glUniform1f(glGetUniformLocation(prg.getProgramId(), "first"), order == 2 ? 1.0f : 0.0f);
			glUniform1i(glGetUniformLocation(prg.getProgramId(), "transmittanceSampler"), transmittanceUnit);
			glUniform1i(glGetUniformLocation(prg.getProgramId(), "deltaESampler"), deltaEUnit);
			glUniform1i(glGetUniformLocation(prg.getProgramId(), "deltaSRSampler"), deltaSRUnit);
			glUniform1i(glGetUniformLocation(prg.getProgramId(), "deltaSMSampler"), deltaSMUnit);
			for (int layer = 0; layer < RES_R; ++layer) {
				setLayer(prg.getProgramId(), layer);
				drawFullscreenQuad();
			}
		}
		deltaJTexture.stop();

		// computes deltaE (line 8 in algorithm 4.1)
		deltaETexture.start();
		{
			auto& prg = sky.irradianceNProgram.get()->getResource<ShaderProgram_GL>();
			prg.useProgram();
			glUniform1f(glGetUniformLocation(prg.getProgramId(), "first"), order == 2 ? 1.0f : 0.0f);
			glUniform1i(glGetUniformLocation(prg.getProgramId(), "transmittanceSampler"), transmittanceUnit);
			glUniform1i(glGetUniformLocation(prg.getProgramId(), "deltaSRSampler"), deltaSRUnit);
			glUniform1i(glGetUniformLocation(prg.getProgramId(), "deltaSMSampler"), deltaSMUnit);
			drawFullscreenQuad();
		}
		deltaETexture.stop();

		// computes deltaS (line 9 in algorithm 4.1)
		deltaSR_SMTexture.start();
		{
			// turn off color channel 1
			//glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_3D, 0, 0, 0);
			//glDrawBuffer(GL_COLOR_ATTACHMENT0);

			auto& prg = sky.inscatterNProgram.get()->getResource<ShaderProgram_GL>();
			prg.useProgram();
			glUniform1f(glGetUniformLocation(prg.getProgramId(), "first"), order == 2 ? 1.0f : 0.0f);
			glUniform1i(glGetUniformLocation(prg.getProgramId(), "transmittanceSampler"), transmittanceUnit);
			glUniform1i(glGetUniformLocation(prg.getProgramId(), "deltaJSampler"), deltaJUnit);
			for (int layer = 0; layer < RES_R; ++layer) {
				setLayer(prg.getProgramId(), layer);
				drawFullscreenQuad();
			}
		}
		deltaSR_SMTexture.stop();

		glEnable(GL_BLEND);
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

		// adds deltaE into irradiance texture E (line 10 in algorithm 4.1)
		irradianceTexture.start();
		{
			auto& prg = sky.copyIrradianceProgram.get()->getResource<ShaderProgram_GL>();
			prg.useProgram();
			glUniform1f(glGetUniformLocation(prg.getProgramId(), "k"), 1.0);
			glUniform1i(glGetUniformLocation(prg.getProgramId(), "deltaESampler"), deltaEUnit);
			drawFullscreenQuad();
		}
		irradianceTexture.stop();

		// adds deltaS into inscatter texture S (line 11 in algorithm 4.1)
		inscatterTexture.start();
		{
			auto& prg = sky.copyInscatterNProgram.get()->getResource<ShaderProgram_GL>();
			prg.useProgram();
			glUniform1i(glGetUniformLocation(prg.getProgramId(), "deltaSSampler"), deltaSRUnit);
			for (int layer = 0; layer < RES_R; ++layer) {
				setLayer(prg.getProgramId(), layer);
				drawFullscreenQuad();
			}
		}
		inscatterTexture.stop();

		glDisable(GL_BLEND);
	}
	
	glGetError(); // TEMP, the transmittanceSample uniform location above is returning -1 for some reason???
	ASSERT_GL_ERROR;
}


void setLayer(unsigned int prog, int layer)
{
	double r = layer / (RES_R - 1.0);
	r = r * r;
	r = sqrt(Rg * Rg + r * (Rt * Rt - Rg * Rg)) + (layer == 0 ? 0.01 : (layer == RES_R - 1 ? -0.001 : 0.0));
	double dmin = Rt - r;
	double dmax = sqrt(r * r - Rg * Rg) + sqrt(Rt * Rt - Rg * Rg);
	double dminp = r - Rg;
	double dmaxp = sqrt(r * r - Rg * Rg);
	glUniform1f(glGetUniformLocation(prog, "r"), float(r));
	glUniform4f(glGetUniformLocation(prog, "dhdH"), float(dmin), float(dmax), float(dminp), float(dmaxp));
	glUniform1i(glGetUniformLocation(prog, "layer"), layer);
}