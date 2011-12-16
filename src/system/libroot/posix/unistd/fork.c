/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <fork.h>

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <errno_private.h>
#include <locks.h>
#include <libroot_private.h>
#include <runtime_loader.h>
#include <syscalls.h>


typedef struct fork_hook {
	struct fork_hook *next;
	void (*function)(void);
} fork_hook;

#define FORK_LOCK_NAME "fork lock"

static fork_hook *sPrepareHooks, *sParentHooks, *sChildHooks;
static fork_hook *sLastParentHook, *sLastChildHook;
static mutex sForkLock = MUTEX_INITIALIZER(FORK_LOCK_NAME);

extern thread_id __main_thread_id;


/**	Adds a hook to the specified list.
 *	If \a _lastHook is NULL, the hook will be added at the head of the list,
 *	else it will be added at the tail of the list.
 *	Since this function allocates memory, it can fail, and returns B_NO_MEMORY
 *	in that case. It returns B_OK on success.
 */

static status_t
add_fork_hook(fork_hook **_hooks, fork_hook **_lastHook, void (*function)(void))
{
	fork_hook *hook = (fork_hook *)malloc(sizeof(struct fork_hook));
	if (hook == NULL)
		return B_NO_MEMORY;

	hook->function = function;

	if (_lastHook) {
		// add hook at the end of the list

		if (*_hooks == NULL) {
			// first entry of this list
			*_hooks = hook;
			*_lastHook = hook;
		} else {
			// any other item
#if 0
			if (*_lastHook == NULL) {
				// search for last hook (need if an item was added to the beginning only --
				// this can only be the case if this function is called directly, though)
				fork_hook *last = *_hooks;
				while (last->next)
					last = last->next;

				*_lastHook = last;
			}
#endif

			(*_lastHook)->next = hook;
			*_lastHook = hook;
		}

		hook->next = NULL;
	} else {
		// add hook at the beginning of the list
		hook->next = *_hooks;
		*_hooks = hook;
	}

	return B_OK;
}


/**	Calls all hooks in the specified list in ascending order.
 */

static void
call_fork_hooks(fork_hook *hook)
{
	while (hook) {
		hook->function();
		hook = hook->next;
	}
}


/**	Private support function that registers the hooks that will be executed
 *	before and after the team is fork()ed.
 *	It is called from pthread_atfork() and atfork().
 */

status_t
__register_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void))
{
	status_t status = mutex_lock(&sForkLock);
	if (status != B_OK)
		return status;

	if (prepare)
		status = add_fork_hook(&sPrepareHooks, NULL, prepare);

	if (status == B_OK && parent)
		status = add_fork_hook(&sParentHooks, &sLastParentHook, parent);

	if (status == B_OK && child)
		status = add_fork_hook(&sChildHooks, &sLastChildHook, child);

	mutex_unlock(&sForkLock);
	return status;
}


pid_t
fork(void)
{
	thread_id thread;
	status_t status;

	status = mutex_lock(&sForkLock);
	if (status != B_OK)
		return status;

	// call preparation hooks
	call_fork_hooks(sPrepareHooks);

	thread = _kern_fork();
	if (thread < 0) {
		// something went wrong
		mutex_unlock(&sForkLock);
		__set_errno(thread);
		return -1;
	}

	if (thread == 0) {
		// we are the child
		// ToDo: initialize child
		__main_thread_id = find_thread(NULL);
		mutex_init(&sForkLock, FORK_LOCK_NAME);
			// TODO: The lock is already initialized and we in the fork()ing
			// process we should make sure that it is in a consistent state when
			// calling the kernel.
		__gRuntimeLoader->reinit_after_fork();
		__reinit_pwd_backend_after_fork();

		call_fork_hooks(sChildHooks);
	} else {
		// we are the parent
		call_fork_hooks(sParentHooks);
		mutex_unlock(&sForkLock);
	}

	return thread;
}


pid_t
vfork(void)
{
	return fork();
}

