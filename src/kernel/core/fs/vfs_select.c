/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <vfs.h>
#include <fd.h>
#include "vfs_select.h"

#include <sys/select.h>
#include <poll.h>
#include <malloc.h>
#include <string.h>


/** Selects all events in the mask on the specified file descriptor */

static int
select_events(struct select_sync *sync, int fd, int ref, uint16 selectedEvents, bool kernel)
{
	uint32 count = 0;
	uint16 event = 1;

	// select any events asked for
	for (; event < 16; event++) {
		if (selectedEvents & SELECT_FLAG(event)
			&& select_fd(fd, event, ref, sync, kernel) == B_OK)
			count++;
	}
	return count;
}


/** Deselects all events in the mask on the specified file descriptor */

static void
deselect_events(struct select_sync *sync, int fd, uint16 selectedEvents, bool kernel)
{
	uint16 event = 1;

	// deselect any events previously asked for
	for (; event < 16; event++) {
		if (selectedEvents & SELECT_FLAG(event))
			deselect_fd(fd, event, sync, kernel);
	}
}


/** Clears all bits in the fd_set - since we are using variable sized
 *	arrays in the kernel, we can't use the FD_ZERO() macro provided by
 *	sys/select.h for this task.
 *	All other FD_xxx() macros are safe to use, though.
 */

static inline void
fd_zero(fd_set *set, int numfds)
{
	if (set != NULL)
		memset(set, 0, _howmany(numfds, NFDBITS) * sizeof(fd_mask));
}


static int
common_select(int numfds, fd_set *readSet, fd_set *writeSet, fd_set *errorSet,
	bigtime_t timeout, const sigset_t *sigMask, bool kernel)
{
	struct select_sync sync;
	status_t status = B_OK;
	int count = 0;
	int fd;

	// ToDo: set sigMask to make pselect() functional different from select()

	// check if fds are valid before doing anything

	for (fd = 0; fd < numfds; fd++) {
		if (((readSet && FD_ISSET(fd, readSet))
			|| (writeSet && FD_ISSET(fd, writeSet))
			|| (errorSet && FD_ISSET(fd, errorSet)))
			&& !fd_is_valid(fd, kernel))
			return B_FILE_ERROR;
	}

	// allocate resources

	memset(&sync, 0, sizeof(select_sync));

	sync.sem = create_sem(1, "select");
	if (sync.sem < B_OK)
		return sync.sem;

	set_sem_owner(sync.sem, B_SYSTEM_TEAM);

	sync.set = malloc(sizeof(select_info) * numfds);
	if (sync.set == NULL) {
		delete_sem(sync.sem);
		return B_NO_MEMORY;
	}
	sync.count = numfds;

	// start selecting file descriptors

	for (fd = 0; fd < numfds; fd++) {
		sync.set[fd].selected_events = 0;
		sync.set[fd].events = 0;

		if (readSet && FD_ISSET(fd, readSet))
			sync.set[fd].selected_events = SELECT_FLAG(B_SELECT_READ);
		if (writeSet && FD_ISSET(fd, writeSet))
			sync.set[fd].selected_events |= SELECT_FLAG(B_SELECT_WRITE);
		if (errorSet && FD_ISSET(fd, errorSet))
			sync.set[fd].selected_events |= SELECT_FLAG(B_SELECT_ERROR);

		count += select_events(&sync, fd, fd, sync.set[fd].selected_events, kernel);
			// array position is the same as the fd for select()
	}

	if (count < 1) {
		count = B_BAD_VALUE;
		goto err;
	}

	status = acquire_sem_etc(sync.sem, 1,
		B_CAN_INTERRUPT | (timeout != -1 ? B_RELATIVE_TIMEOUT : 0), timeout);

	// deselect file descriptors

	for (fd = 0; fd < numfds; fd++)
		deselect_events(&sync, fd, sync.set[fd].selected_events, kernel);

	// collect the events that are happened in the meantime

	switch (status) {
		case B_OK:
			// Clear sets to store the received events
			// (we don't use the macros, because we have variable sized arrays;
			// the other FD_xxx() macros are safe, though).
			fd_zero(readSet, numfds);
			fd_zero(writeSet, numfds);
			fd_zero(errorSet, numfds);

			for (count = 0, fd = 0;fd < numfds; fd++) {
				if (readSet && sync.set[fd].events & SELECT_FLAG(B_SELECT_READ)) {
					FD_SET(fd, readSet);
					count++;
				}
				if (writeSet && sync.set[fd].events & SELECT_FLAG(B_SELECT_WRITE)) {
					FD_SET(fd, writeSet);
					count++;
				}
				if (errorSet && sync.set[fd].events & SELECT_FLAG(B_SELECT_ERROR)) {
					FD_SET(fd, errorSet);
					count++;
				}
			}
			break;
		case B_INTERRUPTED:
			count = B_INTERRUPTED;
			break;
		default:
			// B_TIMED_OUT, and B_WOULD_BLOCK
			count = 0;
	}

err:
	delete_sem(sync.sem);
	free(sync.set);

	return count;
}


static int
common_poll(struct pollfd *fds, nfds_t numfds, bigtime_t timeout, bool kernel)
{
	status_t status = B_OK;
	int count = 0;
	uint32 i;

	// allocate resources

	select_sync sync;
	memset(&sync, 0, sizeof(select_sync));

	sync.sem = create_sem(1, "poll");
	if (sync.sem < B_OK)
		return sync.sem;

	set_sem_owner(sync.sem, B_SYSTEM_TEAM);

	sync.set = malloc(numfds * sizeof(select_info));
	if (sync.set == NULL) {
		delete_sem(sync.sem);
		return B_NO_MEMORY;
	}
	sync.count = numfds;

	// start polling file descriptors (by selecting them)

	for (i = 0; i < numfds; i++) {
		int fd = fds[i].fd;

		// check if fds are valid
		if (!fd_is_valid(fd, kernel)) {
			fds[i].revents = POLLNVAL;
			continue;
		}

		// initialize events masks
		fds[i].events &= ~POLLNVAL;
		fds[i].revents = 0;
		sync.set[i].selected_events = fds[i].events;
		sync.set[i].events = 0;

		count += select_events(&sync, fd, i, fds[i].events, kernel);
	}

	if (count < 1) {
		count = B_BAD_VALUE;
		goto err;
	}

	status = acquire_sem_etc(sync.sem, 1,
		B_CAN_INTERRUPT | (timeout != -1 ? B_RELATIVE_TIMEOUT : 0), timeout);

	// deselect file descriptors

	for (i = 0; i < numfds; i++)
		deselect_events(&sync, fds[i].fd, sync.set[i].selected_events, kernel);

	// collect the events that are happened in the meantime

	switch (status) {
		case B_OK:
			for (count = 0, i = 0; i < numfds; i++) {
				if (fds[i].revents == POLLNVAL)
					continue;

				// POLLxxx flags and B_SELECT_xxx flags are compatible
				fds[i].revents = sync.set[i].events;
				if (fds[i].revents != 0)
					count++;
			}
			break;
		case B_INTERRUPTED:
			count = B_INTERRUPTED;
			break;
		default:
			// B_TIMED_OUT, and B_WOULD_BLOCK
			count = 0;
	}

err:
	delete_sem(sync.sem);
	free(sync.set);

	return count;
}


//	#pragma mark -
//	public functions exported to the kernel


status_t
notify_select_event(struct selectsync *_sync, uint32 ref, uint8 event)
{
	select_sync *sync = (select_sync *)_sync;

	if (sync == NULL
		|| sync->sem < B_OK
		|| ref > sync->count)
		return B_BAD_VALUE;

	sync->set[ref].events |= SELECT_FLAG(event);

	// only wake up the waiting select()/poll() call if the event
	// match the ones selected
	if (sync->set[ref].selected_events & SELECT_FLAG(event))
		return release_sem(sync->sem);

	return B_OK;
}


//	#pragma mark -
//	Functions called from the POSIX layer


int
sys_select(int numfds, fd_set *readSet, fd_set *writeSet, fd_set *errorSet,
	bigtime_t timeout, const sigset_t *sigMask)
{
	return common_select(numfds, readSet, writeSet, errorSet, timeout, sigMask, true);
}


int
sys_poll(struct pollfd *fds, int numfds, bigtime_t timeout)
{
	return common_poll(fds, numfds, timeout, true);
}


int
user_select(int numfds, fd_set *userReadSet, fd_set *userWriteSet, fd_set *userErrorSet,
	bigtime_t timeout, const sigset_t *userSigMask)
{
	fd_set *readSet = NULL, *writeSet = NULL, *errorSet = NULL;
	uint32 bytes = _howmany(numfds, NFDBITS) * sizeof(fd_mask);
	sigset_t sigMask;
	int result;

	if (numfds < 0)
		return B_BAD_VALUE;

	if ((userReadSet != NULL && !IS_USER_ADDRESS(userReadSet))
		|| (userWriteSet != NULL && !IS_USER_ADDRESS(userWriteSet))
		|| (userErrorSet != NULL && !IS_USER_ADDRESS(userErrorSet))
		|| (userSigMask != NULL && !IS_USER_ADDRESS(userSigMask)))
		return B_BAD_ADDRESS;

	// copy parameters

	if (userReadSet != NULL) {
		readSet = malloc(bytes);
		if (readSet == NULL)
			return B_NO_MEMORY;

		if (user_memcpy(readSet, userReadSet, bytes) < B_OK) {
			result = B_BAD_ADDRESS;
			goto err;
		}
	}

	if (userWriteSet != NULL) {
		writeSet = malloc(bytes);
		if (writeSet == NULL) {
			result = B_NO_MEMORY;
			goto err;
		}
		if (user_memcpy(writeSet, userWriteSet, bytes) < B_OK) {
			result = B_BAD_ADDRESS;
			goto err;
		}
	}

	if (userErrorSet != NULL) {
		errorSet = malloc(bytes);
		if (errorSet == NULL) {
			result = B_NO_MEMORY;
			goto err;
		}
		if (user_memcpy(errorSet, userErrorSet, bytes) < B_OK) {
			result = B_BAD_ADDRESS;
			goto err;
		}
	}

	if (userSigMask != NULL)
		sigMask = *userSigMask;

	result = common_select(numfds, readSet, writeSet, errorSet, timeout, userSigMask ? &sigMask : NULL, false);

	// copy back results

	if (result >= B_OK
		&& ((readSet != NULL && user_memcpy(userReadSet, readSet, bytes) < B_OK)
			|| (writeSet != NULL && user_memcpy(userWriteSet, writeSet, bytes) < B_OK)
			|| (errorSet != NULL && user_memcpy(userErrorSet, errorSet, bytes) < B_OK)))
		result = B_BAD_ADDRESS;

err:
	free(readSet);
	free(writeSet);
	free(errorSet);

	return result;
}


int
user_poll(struct pollfd *userfds, int numfds, bigtime_t timeout)
{
	struct pollfd *fds;
	size_t bytes;
	int result;

	if (numfds < 0)
		return B_BAD_VALUE;

	if (userfds == NULL || !IS_USER_ADDRESS(userfds))
		return B_BAD_ADDRESS;

	// copy parameters

	fds = malloc(bytes = numfds * sizeof(struct pollfd));
	if (fds == NULL)
		return B_NO_MEMORY;

	if (user_memcpy(fds, userfds, bytes) < B_OK) {
		result = B_BAD_ADDRESS;
		goto err;
	}

	result = common_poll(fds, numfds, timeout, false);

	// copy back results
	if (result >= B_OK && user_memcpy(userfds, fds, bytes) < B_OK)
		result = B_BAD_ADDRESS;

err:
	free(fds);

	return result;
}


