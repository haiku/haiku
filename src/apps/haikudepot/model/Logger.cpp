/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "Logger.h"


log_level Logger::fLevel = LOG_LEVEL_INFO;


log_level
Logger::Level()
{
	return fLevel;
}


void
Logger::SetLevel(log_level value)
{
	fLevel = value;
}


/*static*/
const char*
Logger::NameForLevel(log_level value)
{
	switch (value) {
		case LOG_LEVEL_OFF:
			return "off";
		case LOG_LEVEL_INFO:
			return "info";
		case LOG_LEVEL_DEBUG:
			return "debug";
		case LOG_LEVEL_TRACE:
			return "trace";
		case LOG_LEVEL_ERROR:
			return "error";
		default:
			return "?";
	}
}


/*static*/ bool
Logger::SetLevelByName(const char *name)
{
	if (strcmp(name, "off") == 0) {
		fLevel = LOG_LEVEL_OFF;
	} else if (strcmp(name, "info") == 0) {
		fLevel = LOG_LEVEL_INFO;
	} else if (strcmp(name, "debug") == 0) {
		fLevel = LOG_LEVEL_DEBUG;
	} else if (strcmp(name, "trace") == 0) {
		fLevel = LOG_LEVEL_TRACE;
	} else if (strcmp(name, "error") == 0) {
		fLevel = LOG_LEVEL_ERROR;
	} else {
		return false;
	}

	return true;
}


/*static*/
bool
Logger::IsLevelEnabled(log_level value)
{
	return fLevel >= value;
}


bool
Logger::IsInfoEnabled()
{
	return IsLevelEnabled(LOG_LEVEL_INFO);
}


bool
Logger::IsDebugEnabled()
{
	return IsLevelEnabled(LOG_LEVEL_DEBUG);
}


bool
Logger::IsTraceEnabled()
{
	return IsLevelEnabled(LOG_LEVEL_TRACE);
}