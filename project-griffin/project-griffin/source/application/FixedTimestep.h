#pragma once
#ifndef GRIFFIN_FIXEDTIMESTEP_H_
#define GRIFFIN_FIXEDTIMESTEP_H_

#include <cstdint>
#include <functional>
#include <cmath>

namespace griffin {

	class FixedTimestep {
	public:
		using UpdateFuncType = std::function<void(const int64_t, const int64_t, const int64_t,
												  const float, const float, const float)>;

		explicit FixedTimestep(float deltaMs,
							   int64_t countsPerMs,
							   UpdateFuncType&& update_) :
			m_deltaMs{ deltaMs },
			m_deltaCounts{ static_cast<int64_t>(deltaMs * countsPerMs) },
			update{ std::forward<UpdateFuncType>(update_) }
		{}

		inline float tick(const int64_t realTime, const int64_t countsPassed, float gameSpeed = 1.0)
		{
			m_accumulator += static_cast<int64_t>(countsPassed * gameSpeed);

			while (m_accumulator >= m_deltaCounts) {
				update(m_virtualTime, m_gameTime, m_deltaCounts, m_deltaMs, m_deltaMs / 1000.0f, gameSpeed);

				m_gameTime += m_deltaCounts;
				m_virtualTime += m_deltaCounts;
				m_accumulator -= m_deltaCounts;
			}
			m_virtualTime = realTime;

			float interpolation = static_cast<float>(m_accumulator) / static_cast<float>(m_deltaCounts);
			return interpolation;
		}

		// Variables
		int64_t	m_accumulator = 0;
		int64_t m_gameTime = 0;
		int64_t m_virtualTime = 0;

		int64_t m_deltaCounts;
		float	m_deltaMs;

		UpdateFuncType update;

	};

}

#endif