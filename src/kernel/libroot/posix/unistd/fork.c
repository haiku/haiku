/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <syscalls.h>
#include <fork.h>

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>


typedef struct fork_hook {
	struct fork_hook *next;
	void (*function)(void);
} fork_hook;

static fork_hook *sPrepareHooks, *sParentHooks, *sChildHooks;
static fork_hook *sLastParentHook, *sLastChildHook;
static sem_id sForkLock;


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


status_t
__init_fork(void)
{
	sForkLock = create_sem(1, "fork lock");
	if (sForkLock < B_OK)
		return sForkLock;

	return B_OK;
}


/**	Private support function that registers the hooks that will be executed
 *	before and after the team is fork()ed.
 *	It is called from pthread_atfork() and atfork().
 */

status_t
__register_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void))
{
	status_t status;

	while ((status = acquire_sem(sForkLock)) == B_INTERRUPTED);
	if (status != B_OK)
		return status;

	if (prepare)
		status = add_fork_hook(&sPrepareHooks, NULL, prepare);

	if (status == B_OK && parent)
		status = add_fork_hook(&sParentHooks, &sLastParentHook, parent);

	if (status == B_OK && child)
		status = add_fork_hook(&sChildHooks, &sLastChildHook, child);

	release_sem(sForkLock);
	return status;
}


pid_t
fork(void)
{
	thread_id thread;
	status_t status;

	while ((status = acquire_sem(sForkLock)) == B_INTERRUPTED);
	if (status != B_OK)
		return status;

	// call preparation hooks
	call_fork_hooks(sPrepareHooks);

	thread = _kern_fork();
	if (thread < 0) {
		// something went wrong
		errno = thread;
		return -1;
	}

	if (thread == 0) {
		// we are the child
		// ToDo: initialize child
		__init_fork();
		call_fork_hooks(sChildHooks);
	} else {
		// we are the parent
		call_fork_hooks(sParentHooks);
		release_sem(sForkLock);
	}

	return thread;
}

