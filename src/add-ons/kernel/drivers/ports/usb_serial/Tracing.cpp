/*
 * Copyright (c) 2007-2008 by Michael Lotz
 * Heavily based on the original usb_serial driver which is:
 *
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 */
#include "Tracing.h"
#include "Driver.h"
#include "USB3.h"

#include <stdio.h> //sprintf
#include <unistd.h> //posix file i/o - create, write, close
#include <Drivers.h>
#include <directories.h>
#include <driver_settings.h>


#if DEBUG
bool gLogEnabled = true;
#else
bool gLogEnabled = false;
#endif

bool gLogToFile = false;
bool gLogAppend = false;
bool gLogFunctionCalls = true;
bool gLogFunctionReturns = true;
bool gLogFunctionResults = true;

static const char *sLogFilePath = kCommonLogDirectory "/" DRIVER_NAME ".log";
static sem_id sLogLock;


void
load_settings()
{
	void *settingsHandle;
	settingsHandle = load_driver_settings(DRIVER_NAME);
	if (settingsHandle == NULL)
		return;

#if !DEBUG
	gLogEnabled = get_driver_boolean_parameter(settingsHandle,
		"debug_output", gLogEnabled, true);
#endif

	gLogToFile = get_driver_boolean_parameter(settingsHandle,
		"debug_output_in_file", gLogToFile, true);
	gLogAppend = !get_driver_boolean_parameter(settingsHandle,
		"debug_output_file_rewrite", !gLogAppend, true);
	gLogFunctionCalls = get_driver_boolean_parameter(settingsHandle,
		"debug_trace_func_calls", gLogFunctionCalls, false);
	gLogFunctionReturns = get_driver_boolean_parameter(settingsHandle,
		"debug_trace_func_returns", gLogFunctionReturns, false);
	gLogFunctionResults = get_driver_boolean_parameter(settingsHandle,
		"debug_trace_func_results", gLogFunctionResults, false);

	unload_driver_settings(settingsHandle);
}


void
create_log_file()
{
	if(!gLogToFile)
		return;

	int flags = O_WRONLY | O_CREAT | (!gLogAppend ? O_TRUNC : 0);
	close(open(sLogFilePath, flags, 0666));
	sLogLock = create_sem(1, DRIVER_NAME"-logging");
}


void
usb_serial_trace(bool force, const char *format, ...)
{
	if (!gLogEnabled && !force)
		return;

	static char buffer[1024];
	char *bufferPointer = buffer;
	if (!gLogToFile) {
		const char *prefix = "\33[32m"DRIVER_NAME":\33[0m ";
		strcpy(bufferPointer, prefix);
		bufferPointer += strlen(prefix);
	}

	va_list argumentList;
	va_start(argumentList, format);
	vsprintf(bufferPointer, format, argumentList);
	va_end(argumentList);

	if (gLogToFile) {
		acquire_sem(sLogLock);
		int fd = open(sLogFilePath, O_WRONLY | O_APPEND);
		write(fd, buffer, strlen(buffer));
		close(fd);
		release_sem(sLogLock);
	} else
		dprintf(buffer);
}


void
trace_termios(struct termios *tios)
{
	TRACE("struct termios:\n"
		"\tc_iflag:  0x%08x\n"
		"\tc_oflag:  0x%08x\n"
		"\tc_cflag:  0x%08x\n"
		"\tc_lflag:  0x%08x\n"
		"\tc_line:   0x%08x\n"
		"\tc_ispeed: 0x%08x\n"
		"\tc_ospeed: 0x%08x\n"
		"\tc_cc[0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x]\n",
		tios->c_iflag, tios->c_oflag, tios->c_cflag, tios->c_lflag,
		tios->c_line,
		tios->c_ispeed, tios->c_ospeed,
		tios->c_cc[0], tios->c_cc[1], tios->c_cc[2], tios->c_cc[3],
		tios->c_cc[4], tios->c_cc[5], tios->c_cc[6], tios->c_cc[7],
		tios->c_cc[8], tios->c_cc[9], tios->c_cc[10]);
}
