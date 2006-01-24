/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYSLOG_DAEMON_H
#define SYSLOG_DAEMON_H


#include <OS.h>


#define SYSLOG_PORT_NAME		"syslog_daemon"

#define SYSLOG_MESSAGE			'_Syl'
#define SYSLOG_ADD_LISTENER		'aSyl'
#define SYSLOG_REMOVE_LISTENER	'rSyl'


// This message is sent from both, the POSIX syslog API and the kernel's
// dprintf() logging facility if logging to syslog was enabled.

struct syslog_message {
	thread_id	from;
	time_t		when;
	int32		options;
	int16		priority;
	char		ident[B_OS_NAME_LENGTH];
	char		message[1];
};

#define SYSLOG_MESSAGE_BUFFER_SIZE	8192
#define SYSLOG_MAX_MESSAGE_LENGTH	(SYSLOG_MESSAGE_BUFFER_SIZE - sizeof(struct syslog_message))

#define SYSLOG_PRIORITY(options)	((options) & 0x7)
#define SYSLOG_FACILITY(options)	((options) & 0x03f8)
#define SYSLOG_FACILITY_INDEX(options)	(SYSLOG_FACILITY(options) >> 3)

#endif	/* SYSLOG_DAEMON_H */
