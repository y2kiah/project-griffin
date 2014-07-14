/**
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

// Static Variables

int64_t Timer::sTimerFreq = 0;
double Timer::sSecondsPerCount = 0;
double Timer::sMillisecondsPerCount = 0;
bool Timer::sInitialized = false;

// Static Functions

void Timer::assertInitialized() {
	assert(sInitialized && "Timer_win32 is not initialized");
}

int64_t Timer::timerFreq()
{
	return sTimerFreq;
}

double Timer::secondsPerCount()
{
	return sSecondsPerCount;
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

double Timer::secondsSince(int64_t startCounts)
{
	assertInitialized();

	int64_t now = 0;
	QueryPerformanceCounter((LARGE_INTEGER *)&now);
	return static_cast<double>(now - startCounts) * sSecondsPerCount;
}

double Timer::secondsBetween(int64_t startCounts, int64_t stopCounts)
{
	assertInitialized();

	return static_cast<double>(stopCounts - startCounts) * sSecondsPerCount;
}

bool Timer::initHighPerfTimer()
{
	if (!sInitialized) {
		SetThreadAffinityMask(GetCurrentThread(), 1);

		// get high performance counter frequency
		BOOL result = QueryPerformanceFrequency((LARGE_INTEGER *)&sTimerFreq);
		if (result == 0 || sTimerFreq == 0) {
			//debugPrintf("Timer::initTimer: QueryPerformanceFrequency failed (error %d)\n", GetLastError());
			return false;
		}

		sSecondsPerCount = 1.0 / static_cast<double>(sTimerFreq);
		sMillisecondsPerCount = sSecondsPerCount * 1000.0;

		// test counter function
		int64_t dummy = 0;
		result = QueryPerformanceCounter((LARGE_INTEGER *)&dummy);
		if (result == 0) {
			//debugPrintf("Timer::initTimer: QueryPerformanceCounter failed (error %d)\n", GetLastError());
			return false;
		}

		sInitialized = true;
	}
	return true;
}

// Member Functions

void Timer::start()
{
	assertInitialized();

	mCountsPassed = 0;
	mMillisecondsPassed = 0;
	mSecondsPassed = 0;

	QueryPerformanceCounter((LARGE_INTEGER *)&mStartCounts);
	
	mStopCounts = mStartCounts;
}

double Timer::stop()
{
	assertInitialized();

	// query the current counts from QPC and GetTickCount64
	QueryPerformanceCounter((LARGE_INTEGER *)&mStopCounts);

	// get time passed since start() according to QPC and GetTickCount64
	mCountsPassed = mStopCounts - mStartCounts;
	
	mSecondsPassed = max(static_cast<double>(mCountsPassed) * sSecondsPerCount, 0.0);
	mMillisecondsPassed = mSecondsPassed * 1000.0;
	
	return mMillisecondsPassed;
}

int64_t Timer::currentCounts() const
{
	assertInitialized();

	int64_t now = 0;
	QueryPerformanceCounter((LARGE_INTEGER *)&now);
	return now - mStartCounts;
}

double Timer::currentSeconds() const
{
	assertInitialized();

	int64_t now = 0;
	QueryPerformanceCounter((LARGE_INTEGER *)&now);
	return static_cast<double>(now - mStartCounts) * sSecondsPerCount;
}

#endif // ifdef _WIN32