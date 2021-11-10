#pragma once
#include <ostream>
enum class LogSeverity : char
{
	Verbose,
	Info,
	NonSevere,
	Severe,
	Critical
};
typedef void (*LogFnDefinition)(LogSeverity severity, const char* message);