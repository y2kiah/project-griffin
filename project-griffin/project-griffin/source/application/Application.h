#pragma once

#include <memory>
#include <resource/ResourceLoader.h>

namespace griffin {

	struct Application {
		resource::ResourceLoaderPtr		resourceLoader;

	};

	Application make_application();

}