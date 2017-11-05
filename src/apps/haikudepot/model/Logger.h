/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#ifndef LOGGER_H
#define LOGGER_H

#include <String.h>
#include <File.h>
#include <Path.h>

#include "PackageInfo.h"


typedef enum log_level {
	LOG_LEVEL_OFF		= 1,
	LOG_LEVEL_INFO		= 2,
	LOG_LEVEL_DEBUG		= 3,
	LOG_LEVEL_TRACE		= 4
} log_level;


class Logger {
public:
	static	log_level			Level();
	static	void				SetLevel(log_level value);
	static	bool				SetLevelByName(const char *name);

	static	bool				IsInfoEnabled();
	static	bool				IsDebugEnabled();
	static	bool				IsTraceEnabled();

private:
	static	log_level			fLevel;
};


#endif // LOGGER_H
