/*
 * Emuxki BeOS Driver for Creative Labs SBLive!/Audigy series
 *
 * Copyright (c) 2002, Jerome Duval (jerome.duval@free.fr)
 *
 * Original code : BeOS Driver for Intel ICH AC'97 Link interface
 * Copyright (c) 2002, Marcus Overhagen <marcus@overhagen.de>
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <KernelExport.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <directories.h>
#include <OS.h>

#include "debug.h"
#include "emuxki.h"


#if DEBUG > 0
static const char *logfile = kCommonLogDirectory "/emuxki.log";
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


void log_create()
{
#if DEBUG > 0
	int fd = open(logfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	const char *text = DRIVER_NAME ", " VERSION "\n";
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
	write(fd,buf,strlen(buf));
	close(fd);
	release_sem(loglock);

	#if DEBUG > 1
		snooze(150000);
	#endif
#endif
}
