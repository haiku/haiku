/*
 * Copyright 2004-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, jerome.duval@free.fr
 *		Marcus Overhagen, marcus@overhagen.de
 *		Jérôme Lévêque, leveque.jerome@gmail.com
 */


#ifndef _DEBUG_ICE1712_H_
#define _DEBUG_ICE1712_H_

//#define ICE1712_VERBOSE
#ifdef ICE1712_VERBOSE
#	define ITRACE(a...) dprintf("ice1712: " a)
#else
#	define ITRACE(a...) (void)0
#endif

//#define ICE1712_VERY_VERBOSE
#ifdef ICE1712_VERY_VERBOSE
#	define ITRACE_VV(a...) ITRACE(a)
#else
#	define ITRACE_VV(a...) (void)0
#endif

#undef ASSERT
#if DEBUG > 0
	#define ASSERT(a)		if (a) {} else \
		dprintf("ASSERT failed! file = %s, line = %d\n",__FILE__,__LINE__)
#else
	#define ASSERT(a)	((void)(0))
#endif

#endif // _DEBUG_ICE1712_H_
