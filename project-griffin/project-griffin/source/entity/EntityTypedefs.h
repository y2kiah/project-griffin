/**
* @file EntityTypedefs.h
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_ENTITYTYPEDEFS_H_
#define GRIFFIN_ENTITYTYPEDEFS_H_

#include <utility/container/handle_map.h>

// entity type id set to highest 15-bit value to deconflict with components
#define EntityId_typeId		32767


namespace griffin {
	namespace entity {
		typedef griffin::Id_T    ComponentId;
		typedef griffin::IdSet_T ComponentIdSet;
		typedef griffin::Id_T    EntityId;
	}

	namespace scene {
		typedef griffin::Id_T    SceneId;
		typedef griffin::Id_T    SceneNodeId;
	}
}

#endif