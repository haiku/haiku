/*
 * netdebug.h
 *
 * Debugging message support. Nothing really network specific about
 * it, but renamed from "debug.h" to distinguish it from other debug.h's
 * that live around the system, and because the *&^%$#@! Metrowerks 
 * compiler can't deal with subdirs in include directives.
 */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _NETDEBUG_H
#define _NETDEBUG_H

#include <BeBuild.h>

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC extern 
#endif

/* 
 * Debug messages can go to a file, to the console, to both or to nowhere.
 * Use debug_setflags() to set.
 */
#define DEBUG_CONSOLE 1
#define DEBUG_FILE 	  2
#define DEBUG_NOFLUSH 4

EXTERNC _IMPEXP_NET void _debug_setflags(unsigned flags);
EXTERNC _IMPEXP_NET unsigned _debug_getflags(void);

#define debug_setflags _debug_setflags
#define debug_getflags _debug_getflags

/* 
 * function     -DDEBUG=?		comment
 * _____________________________________________________
 * wprintf      anything		for warnings
 * dprintf      DEBUG > 0		for debugging messages 
 * ddprintf     DEBUG > 1		for extra debugging messages
 */

EXTERNC _IMPEXP_NET int _wprintf(const char *format, ...);
EXTERNC _IMPEXP_NET int _dprintf(const char *format, ...);

/*
 * Define null function "noprintf"
 */
#define noprintf (void)

#if DEBUG
#define wprintf _dprintf
#define dprintf _dprintf

#if DEBUG > 1
#define ddprintf _dprintf
#else /* DEBUG > 1 */
#define ddprintf noprintf
#endif /* DEBUG > 1 */

#else /* DEBUG */

#define wprintf _wprintf
#define dprintf noprintf
#define ddprintf noprintf

#endif /* DEBUG */

#endif /* _NETDEBUG_H */
