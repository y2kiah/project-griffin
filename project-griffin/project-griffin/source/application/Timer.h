/**
* @author	Jeff Kiah
*/
#pragma once
#ifndef TIMER_H
#define TIMER_H

#include <cstdint>

#define DISCREPANCY_MS_CHECK 2

/**
*
*/
class Timer {
	private:
		// Static Variables
		static int64_t	sTimerFreq;
		static double	sSecondsPerCount;
		static double	sMillisecondsPerCount;
		
		#ifndef NDEBUG
		static bool		sInitialized;
		#endif

		// Member Variables
		int64_t			mStartCounts, mStopCounts, mCountsPassed;	// high res timer, set by QPC on windows
		//uint64_t		mStartTickCount, mStopTickCount;			// low res timer, set by GetTickCount64 on windows to check for QPC jumps
		double			mMillisecondsPassed;
		double			mSecondsPassed;

	public:
		// Static Functions
		static int64_t	timerFreq();
		static double	secondsPerCount();
		static int64_t	queryCounts();
		static int64_t	countsSince(int64_t startCounts);
		static double	secondsSince(int64_t startCounts);
		static double	secondsBetween(int64_t startCounts, int64_t stopCounts);
		static bool		initHighPerfTimer();

		static inline void assertInitialized(); // in debug mode asserts make sure static functions aren't called before init

		// Functions
		inline int64_t	startCounts() const;
		inline int64_t	stopCounts() const;
		inline int64_t	countsPassed() const;
		inline double	millisecondsPassed() const;
		inline double	secondsPassed() const;

		void			start();
		double			stop();
		inline void		reset();
		inline int64_t	currentCounts() const;
		inline double	currentSeconds() const;

		// Constructor
		explicit Timer();
};

#if defined(_WIN32)
#include "impl/Timer_win32.inl"
#endif

#endif // _TIMER_H