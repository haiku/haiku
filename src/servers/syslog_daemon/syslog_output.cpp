/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "syslog_output.h"

#include <stdio.h>


static void
syslog_output(syslog_message &message)
{
	printf("'%s'[%ld]: %s\n", message.ident, message.from, message.message);
}


void
init_syslog_output(SyslogDaemon *daemon)
{
	daemon->AddHandler(syslog_output);
}

