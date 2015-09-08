#if 0 // not sure I want this
#include "../ModelManager_GL.h"
#include <SDL_log.h>


using namespace griffin;
using namespace griffin::render;


// class ModelManager_GL

ModelManager_GL::ModelManager_GL() :
	m_models(0, RESERVE_MODELS)
{}

ModelManager_GL::~ModelManager_GL()
{
	if (m_models.capacity() > RESERVE_MODELS) {
		SDL_Log("check RESERVE_MODELS: original=%d, highest=%d", RESERVE_MODELS, m_models.capacity());
	}
}
#endif