/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_USER_THREAD_DEFS_H
#define _SYSTEM_USER_THREAD_DEFS_H

#include <SupportDefs.h>


struct user_thread {
	int32			defer_signals;		// counter; 0 == signals allowed
	uint32			pending_signals;	// signals that are pending, when
										// signals are deferred
	status_t		wait_status;
};


#endif	/* _SYSTEM_USER_THREAD_DEFS_H */
