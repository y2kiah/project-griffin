/**
 * @file	Profile.cpp
 * @author	Jeff Kiah
 */

#include "../Profile.h"

// Static Variables
Timer Profile::s_timer;
ThreadAggregateMap Profile::s_aggregate;
mutex Profile::s_mutex;
