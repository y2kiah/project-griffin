/* Timer_win32.inl
Author: Jeff Kiah
Orig.Date: 5/21/12
*/
#pragma once
#ifndef TIMER_WIN32_INL
#define TIMER_WIN32_INL

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN	// defined in project settings
#endif

#include "Application/Timer.h"
#include "Windows.h"
#include <crtdbg.h>
#include <cassert>

// Static functions
inline void Timer::assertInitialized()
{
	// debug-only check
	assert(sInitialized && "Timer_win32 is not initialized");
}

// Member functions

// Accessors
inline int64_t	Timer::startCounts() const			{ return mStartCounts; }
inline int64_t	Timer::stopCounts() const			{ return mStopCounts; }
inline int64_t	Timer::countsPassed() const			{ return mCountsPassed; }
inline double	Timer::millisecondsPassed() const	{ return mMillisecondsPassed; }
inline double	Timer::secondsPassed() const		{ return mSecondsPassed; }

inline void Timer::reset()
{
	mStartCounts = mStopCounts = mCountsPassed = 0;
	//mStartTickCount = mStopTickCount = 0;
	mMillisecondsPassed = mSecondsPassed = 0.0f;
}

inline int64_t Timer::currentCounts() const
{
	assertInitialized();

	int64_t now = 0;
	QueryPerformanceCounter((LARGE_INTEGER *)&now);
	return now - mStartCounts;
}

inline double Timer::currentSeconds() const
{
	assertInitialized();

	int64_t now = 0;
	QueryPerformanceCounter((LARGE_INTEGER *)&now);
	return static_cast<double>(now - mStartCounts) * sSecondsPerCount;
}

// Constructor

inline Timer::Timer() :
	mStartCounts(0), mStopCounts(0), mCountsPassed(0), mMillisecondsPassed(0),
	mSecondsPassed(0)//, mStartTickCount(0), mStopTickCount(0)
{}

#endif