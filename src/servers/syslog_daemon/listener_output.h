/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _LISTENER_OUTPUT_H_
#define _LISTENER_OUTPUT_H_

#include <Messenger.h>
#include "SyslogDaemon.h"


void init_listener_output(SyslogDaemon *daemon);
void add_listener(BMessenger *messenger);
void remove_listener(BMessenger *messenger);

#endif	/* _SYSLOG_OUTPUT_H_ */
