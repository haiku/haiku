/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009,10,12 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */

#include <lock.h> // for mutex
#include <stdlib.h> // for file operation
#include <string.h> // for file operation
#include <stdio.h> // for file operation

#include "Settings.h"

bool gTraceOn = false;
bool gTruncateLogFile = false;
bool gAddTimeStamp = true;
bool gTraceFlow = false;
static char *gLogFilePath = NULL;
mutex gLogLock;

static
void create_log()
{
	if (gLogFilePath == NULL)
		return;

	int flags = O_WRONLY | O_CREAT | ((gTruncateLogFile) ? O_TRUNC : 0);
	close(open(gLogFilePath, flags, 0666));

	mutex_init(&gLogLock, DRIVER_NAME"-logging");
}


void load_settings()
{
	void *handle = load_driver_settings(DRIVER_NAME);
	if (handle == 0)
		return;

	gTraceOn = get_driver_boolean_parameter(handle, "trace", gTraceOn, true);
	gTraceFlow = get_driver_boolean_parameter(handle, "trace_flow",
						gTraceFlow, true);
	gTruncateLogFile = get_driver_boolean_parameter(handle,	"truncate_logfile",
						gTruncateLogFile, true);
	gAddTimeStamp = get_driver_boolean_parameter(handle, "add_timestamp",
						gAddTimeStamp, true);
	const char * logFilePath = get_driver_parameter(handle, "logfile",
						NULL, "/var/log/"DRIVER_NAME".log");
	if (logFilePath != NULL) {
		gLogFilePath = strdup(logFilePath);
	}

	unload_driver_settings(handle);

	create_log();
}


void release_settings()
{
	if (gLogFilePath != NULL) {
		mutex_destroy(&gLogLock);
		free(gLogFilePath);
	}
}


void usb_audio_trace(bool force, const char* func, const char *fmt, ...)
{
	if (!(force || gTraceOn)) {
		return;
	}

	va_list arg_list;
	static const char *prefix = DRIVER_NAME":";
	static char buffer[1024];
	char *buf_ptr = buffer;
	if (gLogFilePath == NULL) {
		strcpy(buffer, prefix);
		buf_ptr += strlen(prefix);
	}

	if (gAddTimeStamp) {
		bigtime_t time = system_time();
		uint32 msec = time / 1000;
		uint32 sec = msec / 1000;
		sprintf(buf_ptr, "%02ld.%02ld.%03ld:",
				sec / 60, sec % 60, msec % 1000);
		buf_ptr += strlen(buf_ptr);
	}

	if (func	!= NULL) {
		sprintf(buf_ptr, "%s::", func);
		buf_ptr += strlen(buf_ptr);
	}

	va_start(arg_list, fmt);
	vsprintf(buf_ptr, fmt, arg_list);
	va_end(arg_list);

	if (gLogFilePath == NULL) {
		dprintf(buffer);
		return;
	}

	mutex_lock(&gLogLock);
	int fd = open(gLogFilePath, O_WRONLY | O_APPEND);
	write(fd, buffer, strlen(buffer));
	close(fd);
	mutex_unlock(&gLogLock);
}

