/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
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


bool
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
	} else {
		return false;
	}

	return true;
}


bool
Logger::IsInfoEnabled()
{
	return fLevel >= LOG_LEVEL_INFO;
}


bool
Logger::IsDebugEnabled()
{
	return fLevel >= LOG_LEVEL_DEBUG;
}


bool
Logger::IsTraceEnabled()
{
	return fLevel >= LOG_LEVEL_TRACE;
}