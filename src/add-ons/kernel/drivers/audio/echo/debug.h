/*
 * EchoGals/Echo24 BeOS Driver for Echo audio cards
 *
 * Copyright (c) 2003, Jerome Duval (jerome.duval@free.fr)
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
#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef ECHO24_FAMILY
#define DRIVER_NAME "echo24"
#endif
#ifdef ECHOGALS_FAMILY
#define DRIVER_NAME "echogals"
#endif
#ifdef INDIGO_FAMILY
#define DRIVER_NAME "echoindigo"
#endif
#ifdef ECHO3G_FAMILY
#define DRIVER_NAME "echo3g"
#endif
#define ECHO_VERSION		"0.0"

/*
 * PRINT() executes dprintf if DEBUG = 0 (disabled), or expands to LOG() when DEBUG > 0
 * TRACE() executes dprintf if DEBUG > 0
 * LOG()   executes dprintf and writes to the logfile if DEBUG > 0
 */

/* DEBUG == 0, no debugging, PRINT writes to syslog
 * DEBUG == 1, TRACE & LOG, PRINT 
 * DEBUG == 2, TRACE & LOG, PRINT with snooze()
 */
#ifndef DEBUG
	#define DEBUG 0
#endif

#undef PRINT
#undef TRACE
#undef ASSERT

#if DEBUG > 0
	#define PRINT(a)		log_printf a
	#define TRACE(a) 		debug_printf a
	#define LOG(a)			log_printf a
	#define LOG_CREATE()	log_create()
	#define ASSERT(a)		if (a) {} else LOG(("ASSERT failed! file = %s, line = %d\n",__FILE__,__LINE__))
#ifdef __cplusplus
extern "C" {
#endif

	void log_create(void);
	void log_printf(const char *text,...);
	void debug_printf(const char *text,...);
#ifdef __cplusplus
}
#endif

#else
#ifdef __cplusplus
extern "C" {
#endif
	void log_create(void);
	void debug_printf(const char *text,...);
#ifdef __cplusplus
}
#endif
	#define PRINT(a)	debug_printf a
	#define TRACE(a)	((void)(0))
	#define ASSERT(a)	((void)(0))
	#define LOG(a)		((void)(0))
	#define LOG_CREATE()
#endif

#endif
