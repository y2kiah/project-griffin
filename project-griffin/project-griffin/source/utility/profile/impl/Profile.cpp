/**
 * @file	Profile.cpp
 * @author	Jeff Kiah
 */
#include "../Profile.h"
#include "../ProfileAggregate.h"
#include <application/Timer.h>
#include <utility/concurrency.h>
#include <boost/container/flat_map.hpp>

using namespace std;
using boost::container::flat_map;

namespace griffin {
	// Typedefs

	/** maps address of block's name string (cast to intptr_t), to its ProfileAggregate */
//	typedef flat_map<intptr_t, ProfileAggregate>	ProfileAggregateMap;

	/** maps thread id hash to the ProfileAggregates for the thread */
	//typedef flat_map<size_t, ProfileAggregateMap> ThreadAggregateMap;

	// Variables
	monitor<ProfileAggregateMap> g_aggregates;

	// Class ProfileAggregate
	//#include <SDL_log.h>

	/*ProfileAggregate::ProfileAggregate() {
		SDL_Log("ProfileAggregate created on thread %lu\n", std::this_thread::get_id().hash());
	}

	ProfileAggregate::~ProfileAggregate() {
		SDL_Log("ProfileAggregate destroyed on thread %lu\n", std::this_thread::get_id().hash());
	}*/

	void ProfileAggregate::invoke(int64_t countsPassed, int64_t frame) {
		++m_invocations;
		m_countsCumulative += countsPassed;

		m_countsMax = max(countsPassed, m_countsMax);
		m_countsMin = min(countsPassed, m_countsMin);

		// add new frame
		if (frame > m_lastFrame) {
			++m_frames;

			if (m_collectFrameData) {
				m_frameData.push_back({ frame, 1, countsPassed });
			}

			// update current frame data (another invocation)
		} else {
			if (m_collectFrameData) {
				m_frameData[frame].m_counts += countsPassed;
				++m_frameData[frame].m_invocations;
			}
		}

		m_lastFrame = frame;
		m_firstFrame = min(frame, m_firstFrame);
	}


	void ProfileAggregate::init(const char* name, const char* parentName, string path)
	{
		m_name = name;
		m_parent = parentName;
		m_path = std::move(path);
	}


	// Class Profile

	//template <int Th>
	//Profile<Th>::Profile(int32_t frame, const char* name, const char* file, int line) :
	//	m_frame(frame),
	//	m_name(name),
	//	m_file(file),
	//	m_line(line)
	//{
	//	m_startCounts = Timer::queryCounts();
	//}


	//template <int Th>
	//Profile<Th>::~Profile() {
	//	auto countsPassed = Timer::countsSince(m_startCounts);

	//	//auto threadIdHash = this_thread::get_id().hash();
	//	//auto& threadAggregates = getThreadAggregates(threadIdHash);

	//	g_aggregates([=](ProfileAggregateMap& aggMap) {
	//		// thread safe function
	//		auto size = aggMap.size();
	//		auto& agg = aggMap[reinterpret_cast<intptr_t>(m_name)]; // get or create the block aggregate

	//		if (aggMap.size() == size + 1) { // if newly created
	//			agg.init(m_name, getParentName(), getPath());
	//		}

	//		agg.invoke(countsPassed, m_frame);
	//	});
	//}


	//template <int Th>
	//const char* Profile<Th>::getParentName() const {
	//	const char* parentName = nullptr;

	//	auto s = stack().size();
	//	if (s > 1) {
	//		auto parent = stack()[s - 2];
	//		parentName = parent->m_name;
	//	}

	//	return parentName;
	//}


	//template <int Th>
	//string Profile<Th>::getPath() const {
	//	string path;
	//	path.reserve(100);
	//	for (auto p : stack()) {
	//		path.append(p->m_name);
	//		if (p != this) { path.append("/"); }
	//	}
	//	return std::move(path);
	//}
}