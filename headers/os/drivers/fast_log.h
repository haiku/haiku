/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Fast logging facilities.

	This logging is much faster then dprintf and is saver
	against buffer overflow conditions.

	Clients have to define events by creating an array of 
	fast_log_type entries, each consisting of a code and the
	associated human-readable description. During logging, 
	only the code is stored to save space and time. Also, 
	all arguments are stored binary (currently, they must 
	be of type uint32). Conversion to human-readable text is
	post-poned until the logging data is read or a client
	connection is closed (as this invalidates the event-
	description binding).

	The reader interface is /dev/FAST_LOG_DEVFS_NAME. This
	device can only be read from (no seeking or further
	functionality) and does not block if there is no data.
	This is because if you want to store the log into a
	file, the file access may lead to further logging data,
	so doing a sleep between reads saves logging entries.
*/

#ifndef FAST_LOG_H
#define FAST_LOG_H

#include <module.h>

// one event type as defined by client
typedef struct fast_log_event_type {
	// code of event (zero means end-of-list)
	int16 event;
	// human-readable description
	const char *description;
} fast_log_event_type;


// handle of client connection
typedef struct fast_log_connection *fast_log_handle;


// client interface
typedef struct fast_log_info {
	module_info minfo;

	// open client connection
	// prefix - prefix to print in log before each event
	// types  - array of event types; last entry must have event=0
	fast_log_handle (*start_log)(const char *prefix, fast_log_event_type types[]);
	// close client connection
	void (*stop_log)(fast_log_handle handle);

	// log an entry without arguments
	void (*log_0)(fast_log_handle handle, int event);
	// log an entry with one argument
	void (*log_1)(fast_log_handle handle, int event, uint32 param1);
	// log an entry with two arguments
	void (*log_2)(fast_log_handle handle, int event, uint32 param1, uint32 param2);
	// log an entry with three arguments
	void (*log_3)(fast_log_handle handle, int event, uint32 param1, uint32 param2, uint32 param3);
	// log an entry with four arguments
	void (*log_4)(fast_log_handle handle, int event, uint32 param1, uint32 param2, uint32 param3, uint32 param4);

	// log an entry with n arguments
	// don't cheat on num_params!
	void (*log_n)(fast_log_handle handle, int event, int num_params, ...);
} fast_log_info;

// name of client interface module
#define FAST_LOG_MODULE_NAME "generic/fast_log/v1"
// name of device to read logging data from
#define FAST_LOG_DEVFS_NAME "fast_log"

#endif
