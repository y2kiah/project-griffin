/**
 * @file	Entity.cpp
 * @author	Jeff Kiah
 */
#include <mutex>
#include <memory>
#include <component/Entity.h>

using namespace std;

shared_ptr<Entity> EntityFactory::createEntity(ComponentMask componentMask)
{
//	static mutex mut;
//	lock_guard<mutex> hold(mut);
	
	auto esp = make_shared<Entity>();
	/*for (uint8_t ct = 0; ct < componentMask.size(); ++ct) {
		if (componentMask[ct]) {
			esp->addComponent((ComponentType)ct);
		}
	}*/
	
	return esp;
}