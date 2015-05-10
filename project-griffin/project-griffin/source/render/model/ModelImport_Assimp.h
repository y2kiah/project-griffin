#pragma once
#ifdef GRIFFIN_TOOLS_BUILD
#ifndef GRIFFIN_MODEL_IMPORT_ASSIMP_H_
#define GRIFFIN_MODEL_IMPORT_ASSIMP_H_

#include <memory>
#include <string>

namespace griffin {
	namespace render {
		class Mesh_GL;

		extern std::unique_ptr<Mesh_GL> importModelFile(const std::string &filename);
	}
}

#endif
#endif