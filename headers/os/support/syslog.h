/******************************************************************************
/
/	File:			syslog.h
/
/	Description:	System event logging mechanism.
/
/	Copyright 1993-98, Be Incorporated
/
*******************************************************************************/

/*
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)syslog.h	7.20 (Berkeley) 2/23/91
 */

#ifndef _SYS_LOG_H
#define	_SYS_LOG_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif

/*-------------------------------------------------------------*/
/*----- Priority ----------------------------------------------*/

#define	LOG_EMERG	0	
#define LOG_PANIC	LOG_EMERG
#define	LOG_ALERT	1	
#define	LOG_CRIT	2	
#define	LOG_ERR		3	
#define	LOG_WARNING	4	
#define	LOG_NOTICE	5	
#define	LOG_INFO	6	
#define	LOG_DEBUG	7	

#define LOG_NPRIORITIES	8

#define	LOG_PRIMASK	0x07	
#define	LOG_PRI(p)				((p) & LOG_PRIMASK)
#define	LOG_MAKEPRI(fac, pri)	(((fac) << 3) | (pri))

#ifdef SYSLOG_NAMES
#define	INTERNAL_NOPRI	0x8		
#define	INTERNAL_MARK	LOG_MAKEPRI(LOG_NFACILITIES, 0)

static char *prioritynames[] = {
	"panic",					
	"alert",				
	"crit",					
	"err",					
	"warning",					
	"notice",				
	"info",					
	"debug",			
	"none",				
	NULL
};

#endif /* SYSLOG_NAMES */

/*-------------------------------------------------------------*/
/*----- Facility ----------------------------------------------*/

#define	LOG_KERN	(0<<3)	
#define	LOG_USER	(1<<3)	
#define	LOG_MAIL	(2<<3)	
#define	LOG_DAEMON	(3<<3)
#define	LOG_AUTH	(4<<3)	
#define	LOG_SYSLOG	(5<<3)	
#define	LOG_LPR		(6<<3)	
#define	LOG_NEWS	(7<<3)	
#define	LOG_UUCP	(8<<3)	
#define	LOG_CRON	(9<<3)	
#define	LOG_AUTHPRIV	(10<<3)	

/*----- Private or reserved ---------------*/
#define	LOG_LOCAL0	(16<<3)
#define	LOG_LOCAL1	(17<<3)	
#define	LOG_LOCAL2	(18<<3)	
#define	LOG_LOCAL3	(19<<3)	
#define	LOG_LOCAL4	(20<<3)	
#define	LOG_LOCAL5	(21<<3)	
#define	LOG_LOCAL6	(22<<3)	
#define	LOG_LOCAL7	(23<<3)	
/*-----------------------------------------*/

#define	LOG_NFACILITIES	24	
#define	LOG_FACMASK	0x03f8	
#define	LOG_FAC(p)	(((p) & LOG_FACMASK) >> 3)

#ifdef SYSLOG_NAMES

static char *facilitynames[] = {
	"kern", "user", "mail", "daemon",
	"auth", "syslogd", "lpr", "news",
	"uucp", "cron", "authpriv", "",
	"", "", "", "",
	"local0", "local1", "local2", "local3",
	"local4", "local5", "local6", "local7",
	NULL
};

#endif /* SYSLOG_NAMES */

/*-------------------------------------------------------------*/
/*----- Logging  ----------------------------------------------*/

#ifdef KERNEL
#define	LOG_PRINTF	-1	
#endif

/*----- setlogmask args ---------------*/
#define	LOG_MASK(pri)	(1 << (pri))		
#define	LOG_UPTO(pri)	((1 << ((pri)+1)) - 1)	

/*----- openlog options ---------------*/
#define LOG_OPTIONSMASK	0x000ff000
#define LOG_OPTIONS(p)	((p) & LOG_OPTIONSMASK)

#define	LOG_PID		(0x01<<12)	
#define LOG_THID	LOG_PID
#define LOG_TMID	LOG_PID
#define	LOG_CONS	(0x02<<12)	/* no-op */
#define	LOG_ODELAY	(0x04<<12)	/* no-op */
#define	LOG_NDELAY	(0x08<<12)	/* no-op */
#define LOG_SERIAL	(0x10<<12)	
#define	LOG_PERROR	(0x20<<12)	

#ifdef __cplusplus
extern "C" {
#endif

/*----- Logging Functions ---------------*/

_IMPEXP_BE void	closelog (void);
_IMPEXP_BE void	closelog_team (void);
_IMPEXP_BE void	closelog_thread (void);
_IMPEXP_BE void	openlog (char *, int, int);
_IMPEXP_BE void	openlog_team (char *, int, int);
_IMPEXP_BE void	openlog_thread (char *, int, int);
_IMPEXP_BE void	syslog (int, char *, ...);
_IMPEXP_BE void	log_team (int, char *, ...);
_IMPEXP_BE void	log_thread (int, char *, ...);
_IMPEXP_BE int		setlogmask (int);
_IMPEXP_BE int		setlogmask_team (int);
_IMPEXP_BE int		setlogmask_thread (int);

#ifdef __cplusplus
}
#endif

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _SYS_LOG_H */
