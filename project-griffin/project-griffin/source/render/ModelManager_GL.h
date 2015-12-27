#pragma once
#ifndef GRIFFIN_MODELMANAGER_H_
#define GRIFFIN_MODELMANAGER_H_

#include <render/model/Model_GL.h>
#include <utility/container/handle_map.h>


namespace griffin {
	namespace render {

		class ModelManager_GL {
		public:
			typedef griffin::handle_map<Model_GL> ModelMap;

			explicit ModelManager_GL() : m_models(0, 100) {} // TEMP
			~ModelManager_GL() {}

		private:
			// Variables

			ModelMap	m_models;

		};


	}
}

#endif