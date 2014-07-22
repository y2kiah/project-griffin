/**
 * @file	Timer_win32.cpp
 * @author	Jeff Kiah
 */
#include "../Timer.h"

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN	// defined in project settings
#endif

#include <cmath>
#include <cassert>
#include "Windows.h"

using namespace griffin;

// Static Variables

int64_t Timer::s_countsPerSecond = 0;
int64_t Timer::s_countsPerMs = 0;
double Timer::s_secondsPerCount = 0;
double Timer::s_millisPerCount = 0;
bool Timer::s_initialized = false;

// Static Functions

void Timer::assertInitialized() {
	assert(s_initialized && "Timer_win32 is not initialized");
}

int64_t Timer::countsPerSecond()
{
	return s_countsPerSecond;
}

int64_t Timer::countsPerMs()
{
	return s_countsPerMs;
}

double Timer::secondsPerCount()
{
	return s_secondsPerCount;
}

double Timer::millisPerCount()
{
	return s_millisPerCount;
}

int64_t Timer::queryCounts()
{
	assertInitialized();

	int64_t now = 0;
	QueryPerformanceCounter((LARGE_INTEGER *)&now);
	return now;
}

int64_t Timer::countsSince(int64_t startCounts)
{
	assertInitialized();

	int64_t now = 0;
	QueryPerformanceCounter((LARGE_INTEGER *)&now);
	return now - startCounts;
}

double Timer::querySecondsSince(int64_t startCounts)
{
	assertInitialized();

	int64_t now = 0;
	QueryPerformanceCounter((LARGE_INTEGER *)&now);
	return static_cast<double>(now - startCounts) * s_secondsPerCount;
}

double Timer::queryMillisSince(int64_t startCounts)
{
	assertInitialized();

	int64_t now = 0;
	QueryPerformanceCounter((LARGE_INTEGER *)&now);
	return static_cast<double>(now - startCounts) * s_millisPerCount;
}

double Timer::secondsBetween(int64_t startCounts, int64_t stopCounts)
{
	assertInitialized();

	return static_cast<double>(stopCounts - startCounts) * s_secondsPerCount;
}

double Timer::millisBetween(int64_t startCounts, int64_t stopCounts)
{
	assertInitialized();

	return static_cast<double>(stopCounts - startCounts) * s_millisPerCount;
}

bool Timer::initHighPerfTimer()
{
	if (!s_initialized) {
		SetThreadAffinityMask(GetCurrentThread(), 1);

		// get high performance counter frequency
		BOOL result = QueryPerformanceFrequency((LARGE_INTEGER *)&s_countsPerSecond);
		if (result == 0 || s_countsPerSecond == 0) {
			//debugPrintf("Timer::initTimer: QueryPerformanceFrequency failed (error %d)\n", GetLastError());
			return false;
		}

		s_countsPerMs = s_countsPerSecond / 1000;
		s_secondsPerCount = 1.0 / static_cast<double>(s_countsPerSecond);
		s_millisPerCount = s_secondsPerCount * 1000.0;

		// test counter function
		int64_t dummy = 0;
		result = QueryPerformanceCounter((LARGE_INTEGER *)&dummy);
		if (result == 0) {
			//debugPrintf("Timer::initTimer: QueryPerformanceCounter failed (error %d)\n", GetLastError());
			return false;
		}

		s_initialized = true;
	}
	return true;
}

// Member Functions

int64_t Timer::start()
{
	assertInitialized();

	m_countsPassed = 0;
	m_millisPassed = 0;
	m_secondsPassed = 0;

	QueryPerformanceCounter((LARGE_INTEGER *)&m_startCounts);

	m_stopCounts = m_startCounts;
	return m_startCounts;
}

int64_t Timer::stop()
{
	assertInitialized();

	// query the current counts from QPC and GetTickCount64
	QueryPerformanceCounter((LARGE_INTEGER *)&m_stopCounts);

	// get time passed since start() according to QPC and GetTickCount64
	m_countsPassed = max(m_stopCounts - m_startCounts, 0);

	m_secondsPassed = static_cast<double>(m_countsPassed) * s_secondsPerCount;
	m_millisPassed = m_secondsPassed * 1000.0;

	return m_countsPassed;
}

int64_t Timer::queryCountsPassed()
{
	int64_t countsPassed = stop();
	m_startCounts = m_stopCounts;

	return countsPassed;
}

int64_t Timer::queryCurrentCounts() const
{
	assertInitialized();

	int64_t now = 0;
	QueryPerformanceCounter((LARGE_INTEGER *)&now);
	return now - m_startCounts;
}

double Timer::queryCurrentSeconds() const
{
	assertInitialized();

	int64_t now = 0;
	QueryPerformanceCounter((LARGE_INTEGER *)&now);
	return static_cast<double>(now - m_startCounts) * s_secondsPerCount;
}

#endif // ifdef _WIN32