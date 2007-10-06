/*
 * ice1712 BeOS/Haiku Driver for VIA - VT1712 Multi Channel Audio Controller
 *
 * Copyright (c) 2002, Jerome Duval		(jerome.duval@free.fr)
 * Copyright (c) 2003, Marcus Overhagen	(marcus@overhagen.de)
 * Copyright (c) 2007, Jerome Leveque	(leveque.jerome@neuf.fr)
 *
 * All rights reserved
 * Distributed under the terms of the MIT license.
 */

#include <KernelExport.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <OS.h>
#include "debug.h"
#include "ice1712.h"

#if DEBUG > 0
static const char * logfile="/boot/home/"DRIVER_NAME".log";
static sem_id loglock;
#endif

void debug_printf(const char *text,...);
void log_printf(const char *text,...);
void log_create(void);

void debug_printf(const char *text,...)
{
	char buf[1024];
	va_list ap;

	va_start(ap,text);
	vsprintf(buf,text,ap);
	va_end(ap);

	dprintf(DRIVER_NAME ": %s",buf);
}

void log_create(void)
{
#if DEBUG > 0
	int fd = open(logfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	const char *text = "Driver for " DRIVER_NAME ", Version " VERSION "\n=================\n";
	loglock = create_sem(1,"logfile sem");
	write(fd,text,strlen(text));
	close(fd);
#endif
}

void log_printf(const char *text,...)
{
#if DEBUG > 0
	int fd;
	char buf[1024];
	va_list ap;

	va_start(ap,text);
	vsprintf(buf,text,ap);
	va_end(ap);

	dprintf(DRIVER_NAME ": %s",buf);

	acquire_sem(loglock);
	fd = open(logfile, O_WRONLY | O_APPEND);
	if (fd > 0)
	{
		write(fd, buf, strlen(buf));
		close(fd);
	}
	release_sem(loglock);

	#if DEBUG > 1
		snooze(150000);
	#endif
#endif
}

