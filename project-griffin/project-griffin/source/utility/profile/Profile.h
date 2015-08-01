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

#include "ProfileAggregate.h"
#include <utility/concurrency.h>
#include <boost/container/flat_map.hpp>
using boost::container::flat_map;
using namespace std;

// Macros

#if defined(GRIFFIN_PROFILE) && (GRIFFIN_PROFILE == 1)
#define PROFILE_BLOCK(name, frame, thread)	griffin::Profile<thread> profile_(frame, name, __FILE__, __LINE__);
#else
#define PROFILE_BLOCK(...)
#endif

namespace griffin {
	typedef flat_map<intptr_t, ProfileAggregate>	ProfileAggregateMap;
	extern monitor<ProfileAggregateMap> g_aggregates;

	/**
	 * @class Profile
	 * Profile is an RAII-type container meant to be placed on the stack and will time from
	 * creation to destruction.
	 *
	 * Profile must be thread safe but should not introduce locks that increase contention of the
	 * normally running program. For this reason, unique storage is required for each thread that
	 * is profiled, including a separate stack of Profiles and a separate ProfileAggregate. A
	 * master index of references to all thread-specific lists is kept. This can be problematic for
	 * thread pools when the same task can be picked up by any worker thread at random. The
	 * information for one task will be spread over several lists, but you would actually want to
	 * see it combined into one aggregated entry. A post processing step could look at all of the
	 * thread lists and combine split profiles by name, and also indicate how many unique threads
	 * the profile was run on over its lifetime.
	 *
	 * std::this_thread::get_id().hash() is used to index each thread in the master map
	 */
	template <int Th>
	struct Profile : auto_stacker<Profile<Th>> {
		int64_t		m_frame;		//!< frame the code block is called on
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
		Profile(int64_t frame, const char* name, const char* file, int line);

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

	// TEMP

	template <int Th>
	Profile<Th>::Profile(int64_t frame, const char* name, const char* file, int line) :
		m_frame{ frame },
		m_name{ name },
		m_file{ file },
		m_line{ line }
	{
		m_startCounts = Timer::queryCounts();
	}


	template <int Th>
	Profile<Th>::~Profile() {
		auto countsPassed = Timer::countsSince(m_startCounts);

		//auto threadIdHash = this_thread::get_id().hash();
		//auto& threadAggregates = getThreadAggregates(threadIdHash);

		g_aggregates([=](ProfileAggregateMap& aggMap) {
			// thread safe function
			auto size = aggMap.size();
			auto& agg = aggMap[reinterpret_cast<intptr_t>(m_name)]; // get or create the block aggregate

			if (aggMap.size() == size + 1) { // if newly created
				agg.init(m_name, getParentName(), getPath());
			}

			agg.invoke(countsPassed, m_frame);
		});
	}


	template <int Th>
	const char* Profile<Th>::getParentName() const {
		const char* parentName = nullptr;

		auto s = stack().size();
		if (s > 1) {
			auto parent = stack()[s - 2];
			parentName = parent->m_name;
		}

		return parentName;
	}


	template <int Th>
	string Profile<Th>::getPath() const {
		string path;
		path.reserve(100);
		for (auto p : stack()) {
			path.append(p->m_name);
			if (p != this) { path.append("/"); }
		}
		return std::move(path);
	}

}

#endif