/*
 * ES1370 Haiku Driver for ES1370 audio
 *
 * Copyright 2002-2007, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marcus Overhagen, marcus@overhagen.de
 *		Jerome Duval, jerome.duval@free.fr
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

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
	#define DEBUG 1
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
	void log_create();
	void log_printf(const char *text,...);
	void debug_printf(const char *text,...);
#else
	void debug_printf(const char *text,...);
	#define PRINT(a)	debug_printf a
	#define TRACE(a)	((void)(0))
	#define ASSERT(a)	((void)(0))
	#define LOG(a)		((void)(0))
	#define LOG_CREATE()
#endif

#endif
