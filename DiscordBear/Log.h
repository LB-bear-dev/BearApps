#pragma once
#include "LogDefinitions.h"
#include <sstream>
namespace DiscordBear
{
	extern LogFnDefinition LogFn;

	class LogLine : public std::stringstream
	{
	public:
		LogLine(LogSeverity severity):m_severity(severity) {}
		~LogLine()
		{
			LogFn(m_severity, this->str().c_str());
		}

	private: 
		LogSeverity m_severity;
	};

	#define LOG(severity) LogLine(severity) << "<" << __FUNCTION__  << "> "
}