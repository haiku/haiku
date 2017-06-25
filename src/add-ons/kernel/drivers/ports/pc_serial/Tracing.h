/*
 * Copyright (c) 2007-2008 by Michael Lotz
 * Heavily based on the original usb_serial driver which is:
 *
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PC_SERIAL_TRACING_H_
#define _PC_SERIAL_TRACING_H_

void load_settings();
void create_log_file();
void pc_serial_trace(bool force, const char *format, ...);

#define TRACE_ALWAYS(x...) pc_serial_trace(true, x);
#define TRACE(x...) pc_serial_trace(false, x);

extern bool gLogFunctionCalls;
#define TRACE_FUNCALLS(x...) \
	if (gLogFunctionCalls) \
		pc_serial_trace(false, x);

extern bool gLogFunctionReturns;
#define TRACE_FUNCRET(x...) \
	if (gLogFunctionReturns) \
		pc_serial_trace(false, x);

extern bool gLogFunctionResults;
#define TRACE_FUNCRES(func, param) \
	if (gLogFunctionResults) \
		func(param);

void trace_termios(struct termios *tios);

#endif //_PC_SERIAL_TRACING_H_
