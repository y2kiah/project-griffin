#pragma once

#include <memory>
#include <resource/ResourceLoader.h>

namespace griffin {

	struct Application {
		resource::ResourceLoaderPtr		resourceLoader;

	};

	void test_resource_loader(Application& app); // TEMP
	Application make_application();

}