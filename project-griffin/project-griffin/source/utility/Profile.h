/**
 * @file	Profile.h
 * @author	Jeff Kiah
 */
#pragma once
#ifndef PROFILE_H
#define PROFILE_H

#include <cstdint>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <limits>
#include <boost/container/flat_map.hpp>
#include <application/Timer.h>
#include <utility/AutoLister.h>

using std::string;
using std::vector;
using std::mutex;
using std::atomic;
using boost::container::flat_map;

#if defined(PROFILE) && (PROFILE == 1)
#define PROFILE_BLOCK(name, frame)	Profile profile_(frame, name, __FILE__, __LINE__);
#else
#define PROFILE_BLOCK(...)
#endif

struct FrameData {
	int32_t m_frame;		//!< number of the frame being captured
	int32_t m_invocations;	//!< invocations per frame (good indication of the "hotness" of the code block)
	int64_t m_counts;		//!< total counts within the frame
};

class ProfileAggregate {
public:

	void invoke(int64_t countsPassed, int32_t frame) {
		++m_invocations;
		m_countsCumulative += countsPassed;

		m_countsMax = std::max(countsPassed, m_countsMax);
		m_countsMin = std::min(countsPassed, m_countsMin);

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
		m_firstFrame = std::min(frame, m_firstFrame);
	}

private:
	// Member Variables

	int32_t	m_frames = 0;			//!< total frames that this code block has been run in
	int32_t	m_invocations = 0;		//!< total invocations of the code block
	bool	m_collectFrameData = false; //!< true to turn on frame data collection

	int64_t	m_countsCumulative = 0;	//!< cumulative counts of all invocations
	int64_t	m_countsMax = 0;
	int64_t	m_countsMin = std::numeric_limits<uint64_t>::max();
	
	double	m_msDeviationSqCumulative = 0.0; //!< cumulative of the squared deviation from the mean,
											 //!< used to calc std.dev.

	int32_t	m_firstFrame = std::numeric_limits<uint32_t>::max(); //!< frame number of the first invocation
	int32_t	m_lastFrame  = std::numeric_limits<uint32_t>::min(); //!< frame number of the last invocation

	// track something like framesSkipped or distance between frames, so we can find code that is
	// expensive but runs infrequently possibly causing stutters

	vector<FrameData> m_frameData;	//!< should be able to turn on/off the capture of per-frame
									//!< data at runtime

	// calculated data
	// * msPerFrameAvg
	// * msPerInvocationAvg
	// * msPerFrameStdDev
	// * msPerInvocationStdDev
	// * invocationsPerFrameAvg
	// * framesSkippedAvg
	// * %total frame
	// * %parent frame
};

/**
 * maps thread id hash to the ProfileAggregates for the thread
 */
typedef flat_map<string, ProfileAggregate>		ProfileAggregateMap;
typedef flat_map<size_t, ProfileAggregateMap>	ThreadAggregateMap;


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
struct Profile : AutoStacker<Profile> {
	int32_t	m_frame;		//!< frame the code block is called on
	int64_t	m_startCounts;	//!< counts at start of profiling block
	string	m_name;			//!< name of this profile
	string	m_file;			//!< source file (from macro)
	int		m_line;			//!< line number of profile start (from macro)

	static Timer s_timer;
	static ThreadAggregateMap s_aggregate;
	static mutex s_mutex;

	/**
	 * Constructor
	 * @param	frame	frame number of profiling step
	 */
	Profile(int32_t frame, const char* name, const char* file, int line) :
		m_frame(frame),
		m_name(name),
		m_file(file),
		m_line(line)
	{
		m_startCounts = s_timer.queryCounts();
	}

	/**
	 * Destructor
	 */
	~Profile() {
		auto countsPassed = s_timer.countsSince(m_startCounts);
		
		auto threadIdHash = std::this_thread::get_id().hash();
		auto threadAggregates = getThreadAggregates(threadIdHash);
		
		string path;
		path.reserve(100);
		for (auto p : stack()) {
			path.append(p->m_name).append("/");
		}
		path.append(m_name);

		threadAggregates[path].invoke(countsPassed, m_frame);
	}

	static ProfileAggregateMap& getThreadAggregates(size_t threadIdHash)
	{
		// look into using the DCL (double checked locking) pattern since only writes are once per thread, mostly reads
		// http://preshing.com/20130930/double-checked-locking-is-fixed-in-cpp11/

		std::lock_guard<mutex> lock(s_mutex);
		return s_aggregate[threadIdHash];
	}
};

#endif