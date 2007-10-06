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
#undef DEBUG

#ifndef DEBUG
//	#define DEBUG 1
#endif

#undef PRINT_ICE
#undef TRACE_ICE
#undef ASSERT_ICE

#if DEBUG > 0
	#define PRINT_ICE(a)		log_printf a
//	#define TRACE_ICE(a) 		debug_printf a
	#define TRACE_ICE(a) 		log_printf a
	#define LOG_ICE(a)			log_printf a
	#define LOG_CREATE_ICE()	log_create()
	#define ASSERT_ICE(a)		if (a) {} else LOG_ICE(("ASSERT failed! file = %s, line = %d\n",__FILE__,__LINE__))
	void log_create();
	void log_printf(const char *text,...);
	void debug_printf(const char *text,...);
#else
	void debug_printf(const char *text,...);
	#define PRINT_ICE(a)	debug_printf a
	#define TRACE_ICE(a)	((void)(0))
	#define ASSERT_ICE(a)	((void)(0))
	#define LOG_ICE(a)		((void)(0))
	#define LOG_CREATE_ICE()
#endif

#endif
