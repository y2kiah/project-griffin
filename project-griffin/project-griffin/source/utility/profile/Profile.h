/**
 * @file	Profile.h
 * @author	Jeff Kiah
 */
#pragma once
#ifndef GRIFFIN_PROFILE_H
#define GRIFFIN_PROFILE_H

#include <cstdint>
#include <string>
#include <utility/auto_lister.h>

// Macros

#if defined(GRIFFIN_PROFILE) && (GRIFFIN_PROFILE == 1)
#define PROFILE_BLOCK(name, frame)	griffin::Profile profile_(frame, name, __FILE__, __LINE__);
#else
#define PROFILE_BLOCK(...)
#endif

namespace griffin {

	/**
	 * @class Profile
	 * Profile is an RAII-type container meant to be placed on the stack and will time from creation
	 * to destruction. Each time the profile is run
	 *
	 * Profile must be thread safe but should not introduce locks that increase contention of the
	 * normally running program. For this reason, unique storage is required for each thread that is
	 * profiled, including a separate stack of Profiles and a separate ProfileAggregate. A master index
	 * of references to all thread-specific lists is kept. This can be problematic for thread pools
	 * when the same task can be picked up by any worker thread at random. The aggregate information
	 * for the task will be spread over several lists, when presumably you would want to see them
	 * combined into one entry to capture the total aggregate. A post processing step could look at all
	 * of the thread aggregates and recombine split profiles by name, and also indicate how many unique
	 * threads the profile was taken on over its lifetime.
	 *
	 * std::this_thread::get_id().hash() is used to index each thread in the master map
	 */
	struct Profile : auto_stacker<Profile> {
		int32_t		m_frame;		//!< frame the code block is called on
		int64_t		m_startCounts;	//!< counts at start of profiling block
		const char*	m_name;			//!< name of this profile
		const char*	m_file;			//!< source file (from macro)
		int			m_line;			//!< line number of profile start (from macro)

		/**
		 * Constructor
		 * @param	frame	frame number of profiling step
		 * @param	name	unique name of this profile block
		 * @param	file	source file of the profile block
		 * @param	line	line number of the profile block start
		 */
		Profile(int32_t frame, const char* name, const char* file, int line);

		/**
		 * Destructor
		 */
		~Profile();

		/**
		 *
		 */
		const char* getParentName() const;

		/**
		 * get the full path of nested profile blocks
		 */
		std::string getPath() const;
	};

}

#endif