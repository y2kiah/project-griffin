/**
* @file EntityTypedefs.h
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_ENTITYTYPEDEFS_H_
#define GRIFFIN_ENTITYTYPEDEFS_H_

#include <utility/container/handle_map.h>

namespace griffin {
	namespace entity {

		typedef griffin::Id_T    ComponentId;
		typedef griffin::IdSet_T ComponentIdSet;
		typedef griffin::Id_T    EntityId;

	}
}

#endif