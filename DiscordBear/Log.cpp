#include "PCH.h"
#include "Log.h"

void NoneLogFn(LogSeverity severity, const char* message) {}
LogFnDefinition DiscordBear::LogFn = &NoneLogFn;
