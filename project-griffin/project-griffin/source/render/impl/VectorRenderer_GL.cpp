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
#	pragma comment( lib, "nanovg_d.lib" )
#else
#	pragma comment( lib, "nanovg.lib" )
#endif


// class VectorRenderer_GL

void griffin::render::VectorRenderer_GL::renderViewport(Viewport& viewport)
{
	nvgBeginFrame(m_nvgContext, viewport.width, viewport.height, 1.0f);

	// vector rendering here
	// TEMP
	nvgFontSize(m_nvgContext, 18.0f);
	nvgFontFace(m_nvgContext, "Open Sans");
	nvgFillColor(m_nvgContext, nvgRGBA(255, 255, 255, 160));

	nvgTextAlign(m_nvgContext, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgText(m_nvgContext, 100, 100, "Hello World", nullptr);


	nvgEndFrame(m_nvgContext);
}


void griffin::render::VectorRenderer_GL::init(int viewportWidth, int viewportHeight)
{
	m_nvgContext = nvgCreateGL3(NVG_STENCIL_STROKES /*| NVG_ANTIALIAS | NVG_DEBUG*/);
	
	if (m_nvgContext == nullptr) {
		throw std::runtime_error("nanovg initialization failed");
	}

	m_font = nvgCreateFont(m_nvgContext, "icons", "data/fonts/OpenSans-Regular.ttf");
	if (m_font == -1) {
		throw std::runtime_error("failed to load font");
	}
}


griffin::render::VectorRenderer_GL::~VectorRenderer_GL()
{
	if (m_nvgContext != nullptr) {
		nvgDeleteGL3(m_nvgContext);
	}
}
