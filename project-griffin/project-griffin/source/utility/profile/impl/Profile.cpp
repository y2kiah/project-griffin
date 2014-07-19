/**
 * @file	Profile.cpp
 * @author	Jeff Kiah
 */
#include "../Profile.h"
#include "../ProfileAggregate.h"
#include <application/Timer.h>

#include <mutex>

using namespace std;

namespace griffin {

	// Variables
	Timer g_timer;	//!< high speed timer used by all profiling blocks
	static ThreadAggregateMap g_aggregate;
	static mutex g_mutex;

	// Free Functions

	/**
	 *
	 */
	const ProfileAggregateMap& getThreadAggregates(size_t threadIdHash)
	{
		//static ThreadAggregateMap s_aggregate;
		//static mutex s_mutex;

		// look into using the DCL (double checked locking) pattern since only writes are once per thread, mostly reads
		// http://preshing.com/20130930/double-checked-locking-is-fixed-in-cpp11/

		lock_guard<mutex> lock(g_mutex);
		return g_aggregate[threadIdHash];
	}


	// Class ProfileAggregate
	#include <SDL_log.h>

	ProfileAggregate::ProfileAggregate() {
		SDL_Log("ProfileAggregate created on thread %lu\n", std::this_thread::get_id().hash());
	}

	ProfileAggregate::~ProfileAggregate() {
		SDL_Log("ProfileAggregate destroyed on thread %lu\n", std::this_thread::get_id().hash());
	}

	void ProfileAggregate::invoke(int64_t countsPassed, int32_t frame) {
		++m_invocations;
		m_countsCumulative += countsPassed;

		m_countsMax = max(countsPassed, m_countsMax);
		m_countsMin = min(countsPassed, m_countsMin);

		// add new frame
		if (frame > m_lastFrame) {
			++m_frames;

			if (m_collectFrameData) {
				m_frameData.push_back({frame, 1, countsPassed});
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

	Profile::Profile(int32_t frame, const char* name, const char* file, int line) :
		m_frame(frame),
		m_name(name),
		m_file(file),
		m_line(line)
	{
		m_startCounts = g_timer.queryCounts();
	}

	Profile::~Profile() {
		auto countsPassed = g_timer.countsSince(m_startCounts);

		auto threadIdHash = this_thread::get_id().hash();
		auto threadAggregates = getThreadAggregates(threadIdHash);

		auto size = threadAggregates.size();
		auto agg = threadAggregates[reinterpret_cast<intptr_t>(m_name)]; // get or create the block aggregate
		
		if (threadAggregates.size() == size + 1) { // if newly created
			agg.init(m_name, getParentName(), getPath());
		}

		agg.invoke(countsPassed, m_frame);
	}


	const char* Profile::getParentName() const {
		const char* parentName = nullptr;

		auto s = stack().size();
		if (s > 1) {
			auto parent = stack()[s - 2];
			parentName = parent->m_name;
		}

		return parentName;
	}


	string Profile::getPath() const {
		string path;
		path.reserve(100);
		for (auto p : stack()) {
			path.append(p->m_name);
			if (p != this) { path.append("/"); }
		}
		return std::move(path);
	}
}