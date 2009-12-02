/*
 * Copyright 2009, Krzysztof Ćwiertnia (krzysiek.bmkx_gmail_com).
 *
 * All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ALI_DEBUG_H
#define _ALI_DEBUG_H


#ifdef TRACE
#undef TRACE
#endif

#define TRACE_ALI

#ifdef TRACE_ALI
#define TRACE(a...) dprintf("\33[34mali5451:\33[0m " a)
#else
#define TRACE(a...)
#endif

#endif // _ALI_DEBUG_H
