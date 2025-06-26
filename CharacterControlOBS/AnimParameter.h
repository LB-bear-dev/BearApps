#pragma once
#include <ctime>

namespace CharacterControlRender
{
	enum class AnimStatus
	{
		NotRun,
		Running,
		Finished
	};

	template <typename ParameterType>
	class AnimParameter
	{
		using Interpolator = ParameterType (*)(const ParameterType& begin, const ParameterType& end, float time);
	public:
		AnimParameter(const ParameterType& endParam, float deltaTime, Interpolator interpolator)
		{
			m_interpolator = interpolator;

			m_endParam = endParam;
			m_deltaTime = deltaTime;

			m_startTime = -1;

			m_status = AnimStatus::NotRun;
		}

		void Start(const ParameterType& startParam)
		{
			m_startParam = startParam;
			m_startTime = std::chrono::high_resolution_clock::now();
			m_status = AnimStatus::Running;
		}

		void Update()
		{
			float currentTime = (std::chrono::high_resolution_clock::now() - m_startTime) / m_deltaTime;
			if (currentTime < 1.0f)
			{
				return m_interpolator(m_startParam, m_endParam, currentTime);
			}
			else
			{
				m_status = AnimStatus::Finished;
				return m_endParam;
			}
		}

		bool IsStarted()
		{
			return m_status != AnimStatus::NotRun;
		}

		bool IsRunning()
		{
			return m_status == AnimStatus::Running;
		}

		bool IsFinished()
		{
			return m_status == AnimStatus::Finished;
		}

	private:

		AnimStatus m_status;

		ParameterType m_startParam;
		ParameterType m_endParam;

		int m_memoryOffset;
		Interpolator m_interpolator;

		std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
		float m_deltaTime;
	};
}