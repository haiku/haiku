/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIBROOT_USER_THREAD_H
#define _LIBROOT_USER_THREAD_H

#include <OS.h>
#include <TLS.h>

#include <tls.h>
#include <user_thread_defs.h>


static inline user_thread*
get_user_thread()
{
	return (user_thread*)tls_get(TLS_USER_THREAD_SLOT);
}


static void inline
defer_signals()
{
	get_user_thread()->defer_signals++;
}


static void inline
undefer_signals()
{
	user_thread* thread = get_user_thread();
	if (--thread->defer_signals == 0 && thread->pending_signals != 0) {
		// signals shall no longer be deferred -- call a dummy syscall to handle
		// the pending ones
		is_computer_on();
	}
}


#endif	/* _LIBROOT_USER_THREAD_H */
