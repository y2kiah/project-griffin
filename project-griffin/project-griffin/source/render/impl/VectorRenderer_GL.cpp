/**
* @file VectorRenderer_GL.cpp
* @author Jeff Kiah
*/
#include "../Render.h"
#include <GL/glew.h>
#include <nanovg.h>
#ifndef NANOVG_GL3_IMPLEMENTATION
#define NANOVG_GL3_IMPLEMENTATION
#endif
#include <nanovg_gl.h>

#ifdef _DEBUG
#	pragma comment(lib, "freetype261d.lib")
#else
#	pragma comment(lib, "freetype261.lib")
#endif

// Global Variables

int griffin::render::g_fonts[griffin::render::FontFaceCount] = {};


// free functions, TEMP, some of these will go into a game-specific file for drawing GUI

void drawWindow(NVGcontext* nvg, const char* title, float x, float y, float w, float h)
{
	using namespace griffin::render;

	float cornerRadius = 3.0f;
	
	nvgSave(nvg);
	//	nvgClearState(vg);

	// Window
	nvgBeginPath(nvg);
	nvgRoundedRect(nvg, x, y, w, h, cornerRadius);
	nvgFillColor(nvg, nvgRGBA(28, 30, 34, 192));
	//	nvgFillColor(vg, nvgRGBA(0,0,0,128));
	nvgFill(nvg);

	// Drop shadow
	NVGpaint shadowPaint = nvgBoxGradient(nvg, x, y + 2, w, h, cornerRadius * 2, 10, nvgRGBA(0, 0, 0, 128), nvgRGBA(0, 0, 0, 0));
	nvgBeginPath(nvg);
	nvgRect(nvg, x - 10, y - 10, w + 20, h + 30);
	nvgRoundedRect(nvg, x, y, w, h, cornerRadius);
	nvgPathWinding(nvg, NVG_HOLE);
	nvgFillPaint(nvg, shadowPaint);
	nvgFill(nvg);

	// Header
	NVGpaint headerPaint = nvgLinearGradient(nvg, x, y, x, y + 15, nvgRGBA(255, 255, 255, 8), nvgRGBA(0, 0, 0, 16));
	nvgBeginPath(nvg);
	nvgRoundedRect(nvg, x + 1, y + 1, w - 2, 30, cornerRadius - 1);
	nvgFillPaint(nvg, headerPaint);
	nvgFill(nvg);
	nvgBeginPath(nvg);
	nvgMoveTo(nvg, x + 0.5f, y + 0.5f + 30);
	nvgLineTo(nvg, x + 0.5f + w - 1, y + 0.5f + 30);
	nvgStrokeColor(nvg, nvgRGBA(0, 0, 0, 32));
	nvgStroke(nvg);

	nvgFontSize(nvg, 18.0f);
	nvgFontFaceId(nvg, getFontId(ExoBold));
	nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

	nvgFontBlur(nvg, 2);
	nvgFillColor(nvg, nvgRGBA(0, 0, 0, 128));
	nvgText(nvg, x + 9, y + 16 + 1, title, NULL);

	nvgFontBlur(nvg, 0);
	nvgFillColor(nvg, nvgRGBA(220, 220, 220, 160));
	nvgText(nvg, x + 9, y + 16, title, NULL);

	nvgRestore(nvg);
}



// class VectorRenderer_GL

void griffin::render::VectorRenderer_GL::renderViewport(Viewport& viewport)
{
	glClear(GL_STENCIL_BUFFER_BIT);

	nvgBeginFrame(m_nvg, viewport.width, viewport.height, 1.0f);

	// vector rendering here
	// TEMP
	nvgFontSize(m_nvg, 24.0f);
	nvgFontFaceId(m_nvg, getFontId(Exo));
	nvgFillColor(m_nvg, nvgRGBAf(1.0f, 0.0f, 1.0f, 1.0f));

	nvgTextAlign(m_nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
	nvgText(m_nvg, 7, 4, "FPS:", nullptr);
	
	float cx = 800.0f;
	float cy = 450.0f;
	float r = 100.0f;
	float t = 0.0f;
	{
		float a0 = 0.0f + t * 6;
		float a1 = NVG_PI + t * 6;
		float r0 = r;
		float r1 = r * 0.75f;

		nvgSave(m_nvg);

		nvgBeginPath(m_nvg);
		nvgArc(m_nvg, cx, cy, r0, a0, a1, NVG_CW);
		nvgArc(m_nvg, cx, cy, r1, a1, a0, NVG_CCW);
		nvgClosePath(m_nvg);
		float ax = cx + cosf(a0) * (r0 + r1)*0.5f;
		float ay = cy + sinf(a0) * (r0 + r1)*0.5f;
		float bx = cx + cosf(a1) * (r0 + r1)*0.5f;
		float by = cy + sinf(a1) * (r0 + r1)*0.5f;
		NVGpaint paint = nvgLinearGradient(m_nvg, ax, ay, bx, by, nvgRGBAf(0.0f, 0.5f, 0.0f, 0.0f), nvgRGBAf(0.0f, 0.8f, 0.0f, 0.5f));
		nvgFillPaint(m_nvg, paint);
		nvgFill(m_nvg);

		nvgRestore(m_nvg);
	}

	drawWindow(m_nvg, "Dev Console", 50, 50, 300, 400);

	nvgEndFrame(m_nvg);

	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
}


void griffin::render::VectorRenderer_GL::init(int viewportWidth, int viewportHeight)
{
	m_nvg = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_ANTIALIAS/* | NVG_DEBUG*/);

	if (m_nvg == nullptr) {
		throw std::runtime_error("nanovg initialization failed");
	}
}

void griffin::render::VectorRenderer_GL::loadGlobalFonts(VectorRenderer_GL& inst) {
	// Load fonts
	struct FontDefinition {
		FontFace	face;
		const char*	file;
	};
	FontDefinition fontDefs[] = {
		{ Sans,				"data/fonts/open-sans/OpenSans-Regular.ttf" },
		{ SansBold,			"data/fonts/open-sans/OpenSans-Bold.ttf" },
		{ SansItalic,		"data/fonts/open-sans/OpenSans-Italic.ttf" },
		{ SansBoldItalic,	"data/fonts/open-sans/OpenSans-BoldItalic.ttf" },
		{ Exo,				"data/fonts/exo/Exo-Regular.otf" },
		{ ExoBold,			"data/fonts/exo/Exo-Bold.otf" },
		{ ExoItalic,		"data/fonts/exo/Exo-Italic.otf" },
		{ ExoBoldItalic,	"data/fonts/exo/Exo-BoldItalic.otf" }
	};

	const int numFonts = sizeof(fontDefs) / sizeof(FontDefinition);
	static_assert(numFonts <= FontFaceCount, "too many fonts defined");

	for (int f = 0; f < numFonts; ++f) {
		g_fonts[fontDefs[f].face] = nvgCreateFont(inst.m_nvg, FontFaceToString(fontDefs[f].face), fontDefs[f].file);
		if (g_fonts[fontDefs[f].face] == -1) {
			throw std::runtime_error(std::string("failed to load font ") + std::string(fontDefs[f].file));
		}
	}
	
}


griffin::render::VectorRenderer_GL::~VectorRenderer_GL()
{
	if (m_nvg != nullptr) {
		nvgDeleteGL3(m_nvg);
	}
}
