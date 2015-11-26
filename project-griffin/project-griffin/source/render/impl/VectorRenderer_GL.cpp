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


// free functions

void drawWindow(NVGcontext* vg, const char* title, float x, float y, float w, float h)
{
	float cornerRadius = 3.0f;
	
	nvgSave(vg);
	//	nvgClearState(vg);

	// Window
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, w, h, cornerRadius);
	nvgFillColor(vg, nvgRGBA(28, 30, 34, 192));
	//	nvgFillColor(vg, nvgRGBA(0,0,0,128));
	nvgFill(vg);

	// Drop shadow
	NVGpaint shadowPaint = nvgBoxGradient(vg, x, y + 2, w, h, cornerRadius * 2, 10, nvgRGBA(0, 0, 0, 128), nvgRGBA(0, 0, 0, 0));
	nvgBeginPath(vg);
	nvgRect(vg, x - 10, y - 10, w + 20, h + 30);
	nvgRoundedRect(vg, x, y, w, h, cornerRadius);
	nvgPathWinding(vg, NVG_HOLE);
	nvgFillPaint(vg, shadowPaint);
	nvgFill(vg);

	// Header
	NVGpaint headerPaint = nvgLinearGradient(vg, x, y, x, y + 15, nvgRGBA(255, 255, 255, 8), nvgRGBA(0, 0, 0, 16));
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x + 1, y + 1, w - 2, 30, cornerRadius - 1);
	nvgFillPaint(vg, headerPaint);
	nvgFill(vg);
	nvgBeginPath(vg);
	nvgMoveTo(vg, x + 0.5f, y + 0.5f + 30);
	nvgLineTo(vg, x + 0.5f + w - 1, y + 0.5f + 30);
	nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 32));
	nvgStroke(vg);

	nvgFontSize(vg, 18.0f);
	nvgFontFace(vg, "sans"); //"sans-bold"
	nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

	nvgFontBlur(vg, 2);
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 128));
	nvgText(vg, x + w / 2, y + 16 + 1, title, NULL);

	nvgFontBlur(vg, 0);
	nvgFillColor(vg, nvgRGBA(220, 220, 220, 160));
	nvgText(vg, x + w / 2, y + 16, title, NULL);

	nvgRestore(vg);
}



// class VectorRenderer_GL

void griffin::render::VectorRenderer_GL::renderViewport(Viewport& viewport)
{
	glClear(GL_STENCIL_BUFFER_BIT);

	nvgBeginFrame(m_nvg, viewport.width, viewport.height, 1.0f);

	// vector rendering here
	// TEMP
	nvgFontSize(m_nvg, 24.0f);
	nvgFontFace(m_nvg, "sans");
	nvgFillColor(m_nvg, nvgRGBAf(1.0f, 1.0f, 1.0f, 1.0f));

	nvgTextAlign(m_nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgText(m_nvg, 50, 50, "Hello World", nullptr);

	nvgBeginPath(m_nvg);
	nvgRect(m_nvg, 100, 100, 120, 30);
	nvgFillColor(m_nvg, nvgRGBAf(0.1f, 0.3f, 0.85f, 0.8f));
	nvgFill(m_nvg);
	
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

	drawWindow(m_nvg, "Window Title", 50, 50, 300, 400);

	nvgEndFrame(m_nvg);

	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
}


void griffin::render::VectorRenderer_GL::init(int viewportWidth, int viewportHeight)
{
	m_nvg = nvgCreateGL3(NVG_STENCIL_STROKES/* | NVG_ANTIALIAS | NVG_DEBUG*/);
	
	if (m_nvg == nullptr) {
		throw std::runtime_error("nanovg initialization failed");
	}

	m_font = nvgCreateFont(m_nvg, "sans", "data/fonts/OpenSans-Regular.ttf");
	if (m_font == -1) {
		throw std::runtime_error("failed to load font");
	}
}


griffin::render::VectorRenderer_GL::~VectorRenderer_GL()
{
	if (m_nvg != nullptr) {
		nvgDeleteGL3(m_nvg);
	}
}
