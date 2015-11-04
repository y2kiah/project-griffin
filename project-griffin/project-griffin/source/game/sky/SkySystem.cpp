#include "SkySystem.h"
#include <render/RenderResources.h>


void griffin::game::SkySystem::updateFrameTick(Game* pGame, Engine& engine, const UpdateInfo& ui)
{
	// this function could implement changing sky conditions
}


void griffin::game::SkySystem::init(Game* pGame, const Engine& engine, const SDLApplication& app)
{
	using namespace render;
	using namespace resource;

	auto skyTex = loadTextureCubeMap(L"textures/skybox.dds", CacheType::Cache_Permanent, true);
	skyBoxCubeMap = engine.resourceLoader->getResource(skyTex).get();

}



#include <GL/glew.h>
#include <render/RenderTarget_GL.h>

void precomputeSky()
{
	using namespace griffin::render;

	RenderTarget_GL transmittanceTexture;
	transmittanceTexture.bind(RenderTarget_GL::Albedo_Displacement, GL_TEXTURE0);

	RenderTarget_GL irradianceTexture;
	RenderTarget_GL inscatterTexture;
	RenderTarget_GL deltaETexture;
	RenderTarget_GL deltaSRTexture;
	RenderTarget_GL deltaSMTexture;
	RenderTarget_GL deltaJTexture;

	glActiveTexture(GL_TEXTURE0 + transmittanceUnit);
	glGenTextures(1, &transmittanceTexture);
	glBindTexture(GL_TEXTURE_2D, transmittanceTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F_ARB, TRANSMITTANCE_W, TRANSMITTANCE_H, 0, GL_RGB, GL_FLOAT, NULL);

	glActiveTexture(GL_TEXTURE0 + irradianceUnit);
	glGenTextures(1, &irradianceTexture);
	glBindTexture(GL_TEXTURE_2D, irradianceTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F_ARB, SKY_W, SKY_H, 0, GL_RGB, GL_FLOAT, NULL);

	glActiveTexture(GL_TEXTURE0 + inscatterUnit);
	glGenTextures(1, &inscatterTexture);
	glBindTexture(GL_TEXTURE_3D, inscatterTexture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F_ARB, RES_MU_S * RES_NU, RES_MU, RES_R, 0, GL_RGB, GL_FLOAT, NULL);

	glActiveTexture(GL_TEXTURE0 + deltaEUnit);
	glGenTextures(1, &deltaETexture);
	glBindTexture(GL_TEXTURE_2D, deltaETexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F_ARB, SKY_W, SKY_H, 0, GL_RGB, GL_FLOAT, NULL);

	glActiveTexture(GL_TEXTURE0 + deltaSRUnit);
	glGenTextures(1, &deltaSRTexture);
	glBindTexture(GL_TEXTURE_3D, deltaSRTexture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F_ARB, RES_MU_S * RES_NU, RES_MU, RES_R, 0, GL_RGB, GL_FLOAT, NULL);

	glActiveTexture(GL_TEXTURE0 + deltaSMUnit);
	glGenTextures(1, &deltaSMTexture);
	glBindTexture(GL_TEXTURE_3D, deltaSMTexture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F_ARB, RES_MU_S * RES_NU, RES_MU, RES_R, 0, GL_RGB, GL_FLOAT, NULL);

	glActiveTexture(GL_TEXTURE0 + deltaJUnit);
	glGenTextures(1, &deltaJTexture);
	glBindTexture(GL_TEXTURE_3D, deltaJTexture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F_ARB, RES_MU_S * RES_NU, RES_MU, RES_R, 0, GL_RGB, GL_FLOAT, NULL);

	vector<string> files;
	files.push_back("Main.h");
	files.push_back("common.glsl");
	files.push_back("transmittance.glsl");
	transmittanceProg = loadProgram(files);

	files.clear();
	files.push_back("Main.h");
	files.push_back("common.glsl");
	files.push_back("irradiance1.glsl");
	irradiance1Prog = loadProgram(files);

	files.clear();
	files.push_back("Main.h");
	files.push_back("common.glsl");
	files.push_back("inscatter1.glsl");
	inscatter1Prog = loadProgram(files);

	files.clear();
	files.push_back("Main.h");
	files.push_back("common.glsl");
	files.push_back("copyIrradiance.glsl");
	copyIrradianceProg = loadProgram(files);

	files.clear();
	files.push_back("Main.h");
	files.push_back("common.glsl");
	files.push_back("copyInscatter1.glsl");
	copyInscatter1Prog = loadProgram(files);

	files.clear();
	files.push_back("Main.h");
	files.push_back("common.glsl");
	files.push_back("inscatterS.glsl");
	jProg = loadProgram(files);

	files.clear();
	files.push_back("Main.h");
	files.push_back("common.glsl");
	files.push_back("irradianceN.glsl");
	irradianceNProg = loadProgram(files);

	files.clear();
	files.push_back("Main.h");
	files.push_back("common.glsl");
	files.push_back("inscatterN.glsl");
	inscatterNProg = loadProgram(files);

	files.clear();
	files.push_back("Main.h");
	files.push_back("common.glsl");
	files.push_back("copyInscatterN.glsl");
	copyInscatterNProg = loadProgram(files);

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

	cout << "precomputations..." << endl;

	glGenFramebuffersEXT(1, &fbo);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

	// computes transmittance texture T (line 1 in algorithm 4.1)
	glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, transmittanceTexture, 0);
	glViewport(0, 0, TRANSMITTANCE_W, TRANSMITTANCE_H);
	glUseProgram(transmittanceProg);
	drawQuad();

	// computes irradiance texture deltaE (line 2 in algorithm 4.1)
	glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, deltaETexture, 0);
	glViewport(0, 0, SKY_W, SKY_H);
	glUseProgram(irradiance1Prog);
	glUniform1i(glGetUniformLocation(irradiance1Prog, "transmittanceSampler"), transmittanceUnit);
	drawQuad();

	// computes single scattering texture deltaS (line 3 in algorithm 4.1)
	// Rayleigh and Mie separated in deltaSR + deltaSM
	glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, deltaSRTexture, 0);
	glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, deltaSMTexture, 0);
	unsigned int bufs[2] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT };
	glDrawBuffers(2, bufs);
	glViewport(0, 0, RES_MU_S * RES_NU, RES_MU);
	glUseProgram(inscatter1Prog);
	glUniform1i(glGetUniformLocation(inscatter1Prog, "transmittanceSampler"), transmittanceUnit);
	for (int layer = 0; layer < RES_R; ++layer) {
		setLayer(inscatter1Prog, layer);
		drawQuad();
	}
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_2D, 0, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

	// copies deltaE into irradiance texture E (line 4 in algorithm 4.1)
	glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, irradianceTexture, 0);
	glViewport(0, 0, SKY_W, SKY_H);
	glUseProgram(copyIrradianceProg);
	glUniform1f(glGetUniformLocation(copyIrradianceProg, "k"), 0.0);
	glUniform1i(glGetUniformLocation(copyIrradianceProg, "deltaESampler"), deltaEUnit);
	drawQuad();

	// copies deltaS into inscatter texture S (line 5 in algorithm 4.1)
	glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, inscatterTexture, 0);
	glViewport(0, 0, RES_MU_S * RES_NU, RES_MU);
	glUseProgram(copyInscatter1Prog);
	glUniform1i(glGetUniformLocation(copyInscatter1Prog, "deltaSRSampler"), deltaSRUnit);
	glUniform1i(glGetUniformLocation(copyInscatter1Prog, "deltaSMSampler"), deltaSMUnit);
	for (int layer = 0; layer < RES_R; ++layer) {
		setLayer(copyInscatter1Prog, layer);
		drawQuad();
	}

	// loop for each scattering order (line 6 in algorithm 4.1)
	for (int order = 2; order <= 4; ++order) {

		// computes deltaJ (line 7 in algorithm 4.1)
		glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, deltaJTexture, 0);
		glViewport(0, 0, RES_MU_S * RES_NU, RES_MU);
		glUseProgram(jProg);
		glUniform1f(glGetUniformLocation(jProg, "first"), order == 2 ? 1.0 : 0.0);
		glUniform1i(glGetUniformLocation(jProg, "transmittanceSampler"), transmittanceUnit);
		glUniform1i(glGetUniformLocation(jProg, "deltaESampler"), deltaEUnit);
		glUniform1i(glGetUniformLocation(jProg, "deltaSRSampler"), deltaSRUnit);
		glUniform1i(glGetUniformLocation(jProg, "deltaSMSampler"), deltaSMUnit);
		for (int layer = 0; layer < RES_R; ++layer) {
			setLayer(jProg, layer);
			drawQuad();
		}

		// computes deltaE (line 8 in algorithm 4.1)
		glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, deltaETexture, 0);
		glViewport(0, 0, SKY_W, SKY_H);
		glUseProgram(irradianceNProg);
		glUniform1f(glGetUniformLocation(irradianceNProg, "first"), order == 2 ? 1.0 : 0.0);
		glUniform1i(glGetUniformLocation(irradianceNProg, "transmittanceSampler"), transmittanceUnit);
		glUniform1i(glGetUniformLocation(irradianceNProg, "deltaSRSampler"), deltaSRUnit);
		glUniform1i(glGetUniformLocation(irradianceNProg, "deltaSMSampler"), deltaSMUnit);
		drawQuad();

		// computes deltaS (line 9 in algorithm 4.1)
		glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, deltaSRTexture, 0);
		glViewport(0, 0, RES_MU_S * RES_NU, RES_MU);
		glUseProgram(inscatterNProg);
		glUniform1f(glGetUniformLocation(inscatterNProg, "first"), order == 2 ? 1.0 : 0.0);
		glUniform1i(glGetUniformLocation(inscatterNProg, "transmittanceSampler"), transmittanceUnit);
		glUniform1i(glGetUniformLocation(inscatterNProg, "deltaJSampler"), deltaJUnit);
		for (int layer = 0; layer < RES_R; ++layer) {
			setLayer(inscatterNProg, layer);
			drawQuad();
		}

		glEnable(GL_BLEND);
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

		// adds deltaE into irradiance texture E (line 10 in algorithm 4.1)
		glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, irradianceTexture, 0);
		glViewport(0, 0, SKY_W, SKY_H);
		glUseProgram(copyIrradianceProg);
		glUniform1f(glGetUniformLocation(copyIrradianceProg, "k"), 1.0);
		glUniform1i(glGetUniformLocation(copyIrradianceProg, "deltaESampler"), deltaEUnit);
		drawQuad();

		// adds deltaS into inscatter texture S (line 11 in algorithm 4.1)
		glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, inscatterTexture, 0);
		glViewport(0, 0, RES_MU_S * RES_NU, RES_MU);
		glUseProgram(copyInscatterNProg);
		glUniform1i(glGetUniformLocation(copyInscatterNProg, "deltaSSampler"), deltaSRUnit);
		for (int layer = 0; layer < RES_R; ++layer) {
			setLayer(copyInscatterNProg, layer);
			drawQuad();
		}

		glDisable(GL_BLEND);
	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glFinish();
	cout << "ready." << endl;
	glUseProgram(drawProg);
}