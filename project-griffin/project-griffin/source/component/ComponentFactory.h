#pragma once
#ifndef COMPONENTDATABASE_H
#define COMPONENTDATABASE_H

#include "components.h"
#include "ComponentStore.h"
#include <utility/concurrency.h>

using std::vector;

// think about using the concept idiom that Sean Parent presents here http://www.youtube.com/watch?v=qH6sSOr-yk8

class ComponentDatabase {
public:
	inline ComponentId createComponent(ComponentType ct) {
		return data[ct]->
	}

private:
	vector<std::unique_ptr<ComponentStoreBase>> data;
};



class EntityFactory {
public:
	/*std::future<ComponentId> createComponent(ComponentType ct) {
		// here I need to find the correct component by type
		//newId = STRIP(name)Store.addComponent());

		//return newId;
	}*/

	/*ComponentId createComponent(const std::string &ctStr) {
		ComponentType ct = ComponentTypeToEnum(ctStr);

		//return stores.at(ct).createComponent(ct);
	}*/

	explicit EntityFactory() {}

private:
	concurrent<ComponentDatabase> componentDB;

};

#endif