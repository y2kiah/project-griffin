/**
 * @file	Timer.h
 * @author	Jeff Kiah
 */
#pragma once
#ifndef GRIFFIN_TIMER_H_
#define GRIFFIN_TIMER_H_

#include <cstdint>

namespace griffin {

	/**
	 * @class Timer
	 */
	class Timer {
	public:
		// Static Functions
		static int64_t	countsPerSecond();
		static int64_t	countsPerMs();
		static double	secondsPerCount();
		static double	millisPerCount();
		static int64_t	queryCounts();
		static int64_t	countsSince(int64_t startCounts);
		static double	querySecondsSince(int64_t startCounts);
		static double	queryMillisSince(int64_t startCounts);
		static double	secondsBetween(int64_t startCounts, int64_t stopCounts);
		static double	millisBetween(int64_t startCounts, int64_t stopCounts);
		static bool		initHighPerfTimer();

		static void		assertInitialized(); // asserts when NDEBUG is not defined

		// Functions
		int64_t			startCounts() const			{ return m_startCounts; }
		int64_t			stopCounts() const			{ return m_stopCounts; }
		int64_t			countsPassed() const		{ return m_countsPassed; }
		double			millisPassed() const		{ return m_millisPassed; }
		double			secondsPassed() const		{ return m_secondsPassed; }

		int64_t			start();
		int64_t			stop();
		void			reset() { m_startCounts = m_stopCounts = m_countsPassed = 0; m_millisPassed = m_secondsPassed = 0.0; }
		int64_t			queryCountsPassed();
		int64_t			queryCurrentCounts() const;
		double			queryCurrentSeconds() const;

	private:
		// Static Variables
		static int64_t	s_countsPerSecond;
		static int64_t	s_countsPerMs;
		static double	s_secondsPerCount;
		static double	s_millisPerCount;
		static bool		s_initialized;

		// Member Variables
		int64_t			m_startCounts = 0;
		int64_t			m_stopCounts = 0;
		int64_t			m_countsPassed = 0;	//!< high res timer, set by QPC on windows
		double			m_millisPassed = 0.0;
		double			m_secondsPassed = 0.0;
	};

}

#endif