#pragma once
#ifndef GRIFFIN_FIXEDTIMESTEP_H
#define GRIFFIN_FIXEDTIMESTEP_H

#include <cstdint>
#include <functional>
#include <cmath>

namespace griffin {

	class FixedTimestep {
	public:
		explicit FixedTimestep(double deltaMs,
							   int64_t countsPerMs,
							   const std::function<void(const int64_t, const int64_t, const int64_t, const double)>& update) :
			m_deltaMs(deltaMs),
			m_deltaCounts(static_cast<int64_t>(deltaMs * countsPerMs)),
			update_(update)
		{}

		inline double tick(const int64_t realTime, const int64_t countsPassed, double gameSpeed = 1.0)
		{
			m_accumulator += static_cast<int64_t>(countsPassed * gameSpeed);

			while (m_accumulator >= m_deltaCounts) {
				update_(m_virtualTime, m_gameTime, m_deltaCounts, m_deltaMs);

				m_gameTime += m_deltaCounts;
				m_virtualTime += m_deltaCounts;
				m_accumulator -= m_deltaCounts;
			}
			m_virtualTime = realTime;

			double interpolation = static_cast<double>(m_accumulator) / static_cast<double>(m_deltaCounts);
			return interpolation;
		}

		// Variables
		int64_t	m_accumulator = 0;
		int64_t m_gameTime = 0;
		int64_t m_virtualTime = 0;

		int64_t m_deltaCounts;
		double	m_deltaMs;

		std::function<void(const int64_t, const int64_t, const int64_t, const double)> update_;

	};

}

#endif