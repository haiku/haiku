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
#ifndef _DEBUG_ICE1712_H_
#define _DEBUG_ICE1712_H_

#ifdef TRACE
#	undef TRACE
#endif

//#define ICE1712_VERBOSE
#ifdef ICE1712_VERBOSE
#	define TRACE(a...) dprintf("\33[34mice1712:\33[0m " a)
#else
#	define TRACE(a...) ;
#endif

//#define ICE1712_VERY_VERBOSE
#ifdef ICE1712_VERY_VERBOSE
#	define TRACE_VV(a...) dprintf("\33[34mice1712:\33[0m " a)
#else
#	define TRACE_VV(a...) ;
#endif

#endif // _DEBUG_ICE1712_H_
