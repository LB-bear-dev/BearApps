#pragma once
#include <limits>

namespace CharacterControlOBS
{
	template <typename T>
	class Attribute
	{
	public:

		template <typename T>
		Attribute(T defaultVal, T minVal, T maxVal):
			m_currentValue(defaultVal),
			m_defaultValue(defaultVal),
			m_maxValue(maxVal),
			m_minValue(minVal){}

		template <typename T>
		Attribute(T defaultVal) :
			m_currentValue(defaultVal),
			m_defaultValue(defaultVal),
			m_maxValue(std::numeric_limits<T>::max()),
			m_minValue(std::numeric_limits<T>::lowest()) {}

		void SetValue(T value)
		{
			m_currentValue = std::max(std::min(value, m_maxValue), m_minValue);
		}

		T GetValue() const
		{
			return m_currentValue;
		}
	private:
		T m_currentValue;
		const T m_defaultValue;
		const T m_maxValue;
		const T m_minValue;
	};
}