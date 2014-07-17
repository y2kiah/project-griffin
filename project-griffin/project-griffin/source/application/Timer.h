/**
 * @file	Timer.h
 * @author	Jeff Kiah
 */
#pragma once
#ifndef GRIFFIN_TIMER_H
#define GRIFFIN_TIMER_H

#include <cstdint>

namespace griffin {

	/**
	 * @class Timer
	 */
	class Timer {
	public:
		// Static Functions
		static int64_t	timerFreq();
		static double	secondsPerCount();
		static int64_t	queryCounts();
		static int64_t	countsSince(int64_t startCounts);
		static double	secondsSince(int64_t startCounts);
		static double	secondsBetween(int64_t startCounts, int64_t stopCounts);
		static bool		initHighPerfTimer();

		static void		assertInitialized(); // asserts when NDEBUG is not defined

		// Functions
		int64_t			startCounts() const			{ return mStartCounts; }
		int64_t			stopCounts() const			{ return mStopCounts; }
		int64_t			countsPassed() const		{ return mCountsPassed; }
		double			millisecondsPassed() const	{ return mMillisecondsPassed; }
		double			secondsPassed() const		{ return mSecondsPassed; }

		void			start();
		double			stop();
		void			reset() { mStartCounts = mStopCounts = mCountsPassed = 0; mMillisecondsPassed = mSecondsPassed = 0.0; }
		int64_t			currentCounts() const;
		double			currentSeconds() const;

	private:
		// Static Variables
		static int64_t	sTimerFreq;
		static double	sSecondsPerCount;
		static double	sMillisecondsPerCount;
		static bool		sInitialized;

		// Member Variables
		int64_t			mStartCounts = 0;
		int64_t			mStopCounts = 0;
		int64_t			mCountsPassed = 0;	//!< high res timer, set by QPC on windows
		double			mMillisecondsPassed = 0.0;
		double			mSecondsPassed = 0.0;
	};

}

#endif