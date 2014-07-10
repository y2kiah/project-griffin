#pragma once
#ifndef PROFILE_H
#define PROFILE_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <application/Timer.h>

using std::string;
using std::wstring;
using std::vector;
using std::unordered_map;

struct FrameData {
	uint32_t m_frame = 0;		// number of the frame being captured
	uint32_t m_counts = 0;
	uint32_t m_invocations = 0; // stores invocations per frame (good indication of the "hotness" of the code block)
};

class ProfileAggregate {
public:

	void invoke(uint64_t countsPassed) {
		++m_invocations;
		m_countsCumulative += countsPassed;
	}

private:

	// Member Variables
	uint32_t m_frames = 0;					// total frames that this code block has been run in
	uint32_t m_invocations = 0;				// total invocations of the code block
	uint64_t m_countsCumulative = 0;		// cumulative counts of all invocations

	uint32_t m_countsMax = 0;
	uint32_t m_countsMin = 0xFFFFFFFF;
	
	double m_msDeviationSqCumulative = 0.0;	// cumulative of the squared deviation from the mean, used to calc std.dev.

	uint32_t m_firstFrame = 0;	// counts at the start of the first invocation
	uint32_t m_lastFrame = 0;	// counts at the start of the last invocation
	// track something like framesSkipped or distance between frames, so we can find code that is
	// expensive but runs infrequently possibly causing stutters

	vector<FrameData> m_frames; // should be able to turn on/off the capture of per-frame data at runtime

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

typedef unordered_map<string, ProfileAggregate> AggregateMap;


template <typename T>
class AutoLister {
public:
	AutoLister() {
		m_next = head();
		head() = static_cast<const T*>(this);
	}

	const T* next() const { return m_next; }
	
	static const T*& head() {
		static const T* s_head = 0;
		return s_head;
	}

private:
	const T* m_next;
};

template <typename T>
class AutoStacker {
public:
	AutoStacker() {
		m_stack.emplace_back(this);
	}
	~AutoStacker() {
		assert(m_stack.back() == this && "AutoStacker last element mismatch. AutoStacker must be used as a stack.");
		m_stack.pop_back();
	}

private:
	vector<T*> m_stack;
};

/**
* Profile is an RAII-type container meant to be placed on the stack and will time from creation
* to destruction. Each time the profile is run
*
* Profile must be thread safe but should not introduce locks that increase contention of the
* normally running program. For this reason, unique storage is required for each thread that is
* profiled, including a separate stack of Profiles and a separate ProfileAggregate. A master index
* of references to all thread-specific lists is kept. This can be problematic for thread pools
* when the same task can be picked up by any worker thread at random. The aggregate information for
* the task will be spread over several lists, when presumably you would want to see them combined
* into one entry to capture the total aggregate. A post processing step could look at all of the
* thread aggregates and recombine split profiles by name, and also indicate how many unique threads
* the profile was taken on over its lifetime.
* std::this_thread::get_id().hash() can be used to identify the thread, but how to
*/
struct Profile : AutoLister<Profile> {
	Timer	m_timer;	// cross-platform high perf timer
	string	m_name;		// name of this profile
	wstring	m_file;		// source file (from macro)
	int		m_line;		// line number of profile start (from macro)

	static AggregateMap& s_aggregate;

	Profile() {
		static unordered_map<string, ProfileAggregate> aggregate;
		s_aggregate = aggregate;

		m_timer.start();
	}
	~Profile() {
		m_timer.stop();
		s_aggregate[""].invoke(m_timer.countsPassed());
	}
};

#endif