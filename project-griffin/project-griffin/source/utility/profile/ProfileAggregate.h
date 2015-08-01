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

using std::string;
using std::vector;
using std::deque;
using std::numeric_limits;

namespace griffin {
	/**
	 * @class FrameData
	 */
	struct FrameData {
		int64_t m_frame;		//!< number of the frame being captured
		int64_t m_invocations;	//!< invocations per frame (good indication of the "hotness" of the code block)
		int64_t m_counts;		//!< total counts within the frame
	};


	/**
	 * @class ProfileAggregate
	 */
	class ProfileAggregate {
	public:
		// Functions
		//ProfileAggregate();
		//ProfileAggregate(const ProfileAggregate&) = default;
		//ProfileAggregate(ProfileAggregate&&) = default;
		//~ProfileAggregate();
		

		void invoke(int64_t countsPassed, int64_t frame);

		void init(const char* name, const char* parentName, string path);

		// Member Variables

		int64_t	m_frames = 0;			//!< total frames that this code block has been run in
		int64_t	m_invocations = 0;		//!< total invocations of the code block
		bool	m_collectFrameData = false; //!< true to turn on frame data collection

		int64_t	m_countsCumulative = 0;	//!< cumulative counts of all invocations
		int64_t	m_countsMax = 0;
		int64_t	m_countsMin = numeric_limits<uint64_t>::max();

		double	m_msDeviationSqCumulative = 0.0;	//!< cumulative of the squared deviation from the mean,
													//!< used to calc std.dev.

		int64_t	m_firstFrame = numeric_limits<uint64_t>::max(); //!< frame number of the first invocation
		int64_t	m_lastFrame  = numeric_limits<uint64_t>::min(); //!< frame number of the last invocation

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


	class ProfileAggregateIterator {
	public:
		size_t	nextThread();
		size_t	prevThread();
		size_t	firstThread();
		size_t	lastThread();

	private:
		size_t	m_threadIdHash;

		

	};
}
#endif