/**
 * @file	ProfileAggregate.h
 * @author	Jeff Kiah
 */
#pragma once
#ifndef GRIFFIN_PROFILEAGGREGATE_H
#define GRIFFIN_PROFILEAGGREGATE_H

#include <cstdint>
#include <string>
#include <vector>
#include <deque>
#include <limits>
#include <thread>
#include <boost/container/flat_map.hpp>

using std::string;
using std::vector;
using std::deque;
using std::numeric_limits;
using boost::container::flat_map;

namespace griffin {
	// Typedefs

	class ProfileAggregate;

	/** maps address of block's name string (cast to intptr_t), to its ProfileAggregate */
	typedef flat_map<intptr_t, ProfileAggregate> ProfileAggregateMap;

	/** maps thread id hash to the ProfileAggregates for the thread */
	typedef flat_map<size_t, ProfileAggregateMap> ThreadAggregateMap;

	/**
	 * @class FrameData
	 */
	struct FrameData {
		int32_t m_frame;		//!< number of the frame being captured
		int32_t m_invocations;	//!< invocations per frame (good indication of the "hotness" of the code block)
		int64_t m_counts;		//!< total counts within the frame
	};


	/**
	 * @class ProfileAggregate
	 */
	class ProfileAggregate {
	public:
		// Functions

		/**
		 *
		 */
		void invoke(int64_t countsPassed, int32_t frame);

		/**
		 *
		 */
		void init(const char* name, const char* parentName, string path);

		// Member Variables

		int32_t	m_frames = 0;			//!< total frames that this code block has been run in
		int32_t	m_invocations = 0;		//!< total invocations of the code block
		bool	m_collectFrameData = false; //!< true to turn on frame data collection

		int64_t	m_countsCumulative = 0;	//!< cumulative counts of all invocations
		int64_t	m_countsMax = 0;
		int64_t	m_countsMin = numeric_limits<uint64_t>::max();

		double	m_msDeviationSqCumulative = 0.0;	//!< cumulative of the squared deviation from the mean,
													//!< used to calc std.dev.

		int32_t	m_firstFrame = numeric_limits<uint32_t>::max(); //!< frame number of the first invocation
		int32_t	m_lastFrame  = numeric_limits<uint32_t>::min(); //!< frame number of the last invocation

		const char*	m_name = nullptr;	//!< name of this profile block, same as the map key
		const char*	m_parent = nullptr;	//!< name of the parent block, set on first invocation
		vector<const char*> m_children;	//!< name set on first invocation of each sub-profile
		string		m_path;				//!< full path of the nested profile block

		deque<FrameData> m_frameData;	//!< collects per-frame data when m_collectFrameData is true

		// track something like framesSkipped or distance between frames, so we can find code that is
		// expensive but runs infrequently possibly causing stutters

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
}
#endif