/*
 * Copyright 2005-2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BIN_DEBUG_DEBUG_UTILS_H
#define BIN_DEBUG_DEBUG_UTILS_H

#include <OS.h>


thread_id	load_program(const char* const* args, int32 argCount,
				bool traceLoading);

void		set_team_debugging_flags(port_id nubPort, int32 flags);
void		set_thread_debugging_flags(port_id nubPort, thread_id thread,
				int32 flags);
void		continue_thread(port_id nubPort, thread_id thread);


#endif	// BIN_DEBUG_DEBUG_UTILS_H
