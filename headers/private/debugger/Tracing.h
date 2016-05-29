/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TRACING_H
#define TRACING_H


#include <stdio.h>

#include "apps_debugger_config.h"


#define WARNING(x...)	fprintf(stderr, x)
#define ERROR(x...)		fprintf(stderr, x)


#if APPS_DEBUGGER_TRACE_DWARF_DIE
#	define TRACE_DIE(x...)		printf(x)
#	define TRACE_DIE_ONLY(x)	x
#else
#	define TRACE_DIE(x...)		(void)0
#	define TRACE_DIE_ONLY(x)
#endif

#if APPS_DEBUGGER_TRACE_DWARF_LINE_INFO
#	define TRACE_LINES(x...)	printf(x)
#	define TRACE_LINES_ONLY(x)	x
#else
#	define TRACE_LINES(x...)	(void)0
#	define TRACE_LINES_ONLY(x)
#endif

#if APPS_DEBUGGER_TRACE_DWARF_LINE_INFO >= 2
#	define TRACE_LINES2(x...)	printf(x)
#	define TRACE_LINES2_ONLY(x)	x
#else
#	define TRACE_LINES2(x...)	(void)0
#	define TRACE_LINES2_ONLY(x)
#endif

#if APPS_DEBUGGER_TRACE_DWARF_EXPRESSIONS
#	define TRACE_EXPR(x...)	printf(x)
#	define TRACE_EXPR_ONLY(x)	x
#else
#	define TRACE_EXPR(x...)	(void)0
#	define TRACE_EXPR_ONLY(x)
#endif

#if APPS_DEBUGGER_TRACE_DWARF_PUBLIC_TYPES
#	define TRACE_PUBTYPES(x...)	printf(x)
#	define TRACE_PUBTYPES_ONLY(x)	x
#else
#	define TRACE_PUBTYPES(x...)	(void)0
#	define TRACE_PUBTYPES_ONLY(x)
#endif

#if APPS_DEBUGGER_TRACE_CFI
#	define TRACE_CFI(x...)		printf(x)
#	define TRACE_CFI_ONLY(x)	x
#else
#	define TRACE_CFI(x...)		(void)0
#	define TRACE_CFI_ONLY(x)
#endif

#if APPS_DEBUGGER_TRACE_STACK_FRAME_LOCALS
#	define TRACE_LOCALS(x...)	printf(x)
#	define TRACE_LOCALS_ONLY(x)	x
#else
#	define TRACE_LOCALS(x...)	(void)0
#	define TRACE_LOCALS_ONLY(x)
#endif

#if APPS_DEBUGGER_TRACE_IMAGES
#	define TRACE_IMAGES(x...)	printf(x)
#	define TRACE_IMAGES_ONLY(x)	x
#else
#	define TRACE_IMAGES(x...)	(void)0
#	define TRACE_IMAGES_ONLY(x)
#endif

#if APPS_DEBUGGER_TRACE_CODE
#	define TRACE_CODE(x...)		printf(x)
#	define TRACE_CODE_ONLY(x)	x
#else
#	define TRACE_CODE(x...)		(void)0
#	define TRACE_CODE_ONLY(x)
#endif

#if APPS_DEBUGGER_TRACE_JOBS
#	define TRACE_JOBS(x...)		printf(x)
#	define TRACE_JOBS_ONLY(x)	x
#else
#	define TRACE_JOBS(x...)		(void)0
#	define TRACE_JOBS_ONLY(x)
#endif

#if APPS_DEBUGGER_TRACE_DEBUG_EVENTS
#	define TRACE_EVENTS(x...)	printf(x)
#	define TRACE_EVENTS_ONLY(x)	x
#else
#	define TRACE_EVENTS(x...)	(void)0
#	define TRACE_EVENTS_ONLY(x)
#endif

#if APPS_DEBUGGER_TRACE_TEAM_CONTROL
#	define TRACE_CONTROL(x...)	printf(x)
#	define TRACE_CONTROL_ONLY(x)	x
#else
#	define TRACE_CONTROL(x...)	(void)0
#	define TRACE_CONTROL_ONLY(x)
#endif

#if APPS_DEBUGGER_TRACE_GUI
#	define TRACE_GUI(x...)		printf(x)
#	define TRACE_GUI_ONLY(x)	x
#else
#	define TRACE_GUI(x...)		(void)0
#	define TRACE_GUI_ONLY(x)
#endif


#endif	// TRACING_H
