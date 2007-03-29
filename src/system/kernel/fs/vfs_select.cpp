/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "vfs_select.h"

#include <vfs.h>
#include <fd.h>
#include <syscalls.h>
#include <fs/select_sync_pool.h>

#include <new>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

//#define TRACE_VFS_SELECT
#ifdef TRACE_VFS_SELECT
#	define PRINT(x) dprintf x
#	define FUNCTION(x) dprintf x
#else
#	define PRINT(x) ;
#	define FUNCTION(x) ;
#endif


/*! Selects all events in the mask on the specified file descriptor */
static int
select_events(struct select_sync *sync, int fd, int ref, uint16 selectedEvents,
	bool kernel)
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


/*! Deselects all events in the mask on the specified file descriptor */
static void
deselect_events(struct select_sync *sync, int fd, uint16 selectedEvents,
	bool kernel)
{
	uint16 event = 1;

	// deselect any events previously asked for
	for (; event < 16; event++) {
		if (selectedEvents & SELECT_FLAG(event))
			deselect_fd(fd, event, sync, kernel);
	}
}


/*!
	Clears all bits in the fd_set - since we are using variable sized
	arrays in the kernel, we can't use the FD_ZERO() macro provided by
	sys/select.h for this task.
	All other FD_xxx() macros are safe to use, though.
*/
static inline void
fd_zero(fd_set *set, int numFDs)
{
	if (set != NULL)
		memset(set, 0, _howmany(numFDs, NFDBITS) * sizeof(fd_mask));
}


static int
common_select(int numFDs, fd_set *readSet, fd_set *writeSet, fd_set *errorSet,
	bigtime_t timeout, const sigset_t *sigMask, bool kernel)
{
	struct select_sync sync;
	status_t status = B_OK;
	int fd;

	FUNCTION(("common_select(%d, %p, %p, %p, %lld, %p, %d)\n", numFDs, readSet,
		writeSet, errorSet, timeout, sigMask, kernel));

	// TODO: set sigMask to make pselect() functional different from select()

	// check if fds are valid before doing anything

	for (fd = 0; fd < numFDs; fd++) {
		if (((readSet && FD_ISSET(fd, readSet))
			|| (writeSet && FD_ISSET(fd, writeSet))
			|| (errorSet && FD_ISSET(fd, errorSet)))
			&& !fd_is_valid(fd, kernel))
			return B_FILE_ERROR;
	}

	// allocate resources

	sync.sem = create_sem(0, "select");
	if (sync.sem < B_OK)
		return sync.sem;

	set_sem_owner(sync.sem, B_SYSTEM_TEAM);

	sync.set = (select_info *)malloc(sizeof(select_info) * numFDs);
	if (sync.set == NULL) {
		delete_sem(sync.sem);
		return B_NO_MEMORY;
	}
	sync.count = numFDs;

	// start selecting file descriptors

	for (fd = 0; fd < numFDs; fd++) {
		sync.set[fd].selected_events = 0;
		sync.set[fd].events = 0;

		if (readSet && FD_ISSET(fd, readSet))
			sync.set[fd].selected_events = SELECT_FLAG(B_SELECT_READ);
		if (writeSet && FD_ISSET(fd, writeSet))
			sync.set[fd].selected_events |= SELECT_FLAG(B_SELECT_WRITE);
		if (errorSet && FD_ISSET(fd, errorSet))
			sync.set[fd].selected_events |= SELECT_FLAG(B_SELECT_ERROR);

		select_events(&sync, fd, fd, sync.set[fd].selected_events, kernel);
			// array position is the same as the fd for select()
	}

	status = acquire_sem_etc(sync.sem, 1,
		B_CAN_INTERRUPT | (timeout != -1 ? B_RELATIVE_TIMEOUT : 0), timeout);

	PRINT(("common_select(): acquire_sem_etc() returned: %lx\n", status));

	// deselect file descriptors

	for (fd = 0; fd < numFDs; fd++) {
		deselect_events(&sync, fd, sync.set[fd].selected_events, kernel);
		// TODO: Since we're using FD indices instead of FDs (file_descriptors),
		// it can happen that the FD index (even the FD) on which we invoked
		// the select() FS hook is already closed at this point.
		// This has two main consequences:
		// 1) deselect() is not invoked, if a currently selected FD index is
		//    closed. Thus on close of a FD the FS would need to cleanup the
		//    select resources it had acquired. Harmless.
		// 2) Caused by 1): If the FS close hook invokes notify_select_event()
		//    (which is a nice gesture, though actually undefined by the
		//    POSIX standard), it has no means of synchronization with the
		//    pending select() (since deselect() won't be invoked), i.e. a
		//    second call to notify_select_event() might be too late, since
		//    select() might already be finished. Dangerous!
		//    notify_select_event() would operate on already free()d memory!
	}

	PRINT(("common_select(): events deselected\n"));

	// collect the events that happened in the meantime

	int count = 0;

	if (status == B_INTERRUPTED) {
		// We must not clear the sets in this case, as applications may
		// rely on the contents of them.
		count = B_INTERRUPTED;
		goto err;
	}

	// Clear sets to store the received events
	// (we can't use the macros, because we have variable sized arrays;
	// the other FD_xxx() macros are safe, though).
	fd_zero(readSet, numFDs);
	fd_zero(writeSet, numFDs);
	fd_zero(errorSet, numFDs);

	if (status == B_OK) {
		for (count = 0, fd = 0;fd < numFDs; fd++) {
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
	}

	// B_TIMED_OUT and B_WOULD_BLOCK are supposed to return 0

err:
	delete_sem(sync.sem);
	free(sync.set);

	return count;
}


static int
common_poll(struct pollfd *fds, nfds_t numFDs, bigtime_t timeout, bool kernel)
{
	status_t status = B_OK;
	int count = 0;
	uint32 i;

	// allocate resources

	select_sync sync;
	sync.sem = create_sem(0, "poll");
	if (sync.sem < B_OK)
		return sync.sem;

	set_sem_owner(sync.sem, B_SYSTEM_TEAM);

	sync.set = (select_info*)malloc(numFDs * sizeof(select_info));
	if (sync.set == NULL) {
		delete_sem(sync.sem);
		return B_NO_MEMORY;
	}
	sync.count = numFDs;

	// start polling file descriptors (by selecting them)

	for (i = 0; i < numFDs; i++) {
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

	for (i = 0; i < numFDs; i++) {
		deselect_events(&sync, fds[i].fd, sync.set[i].selected_events, kernel);
			// TODO: same comments apply as on common_select()
	}

	// collect the events that are happened in the meantime

	switch (status) {
		case B_OK:
			for (count = 0, i = 0; i < numFDs; i++) {
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


//	#pragma mark - public kernel API


status_t
notify_select_event(struct selectsync *_sync, uint32 ref, uint8 event)
{
	select_sync *sync = (select_sync *)_sync;

	FUNCTION(("notify_select_event(%p, %lu, %u)\n", _sync, ref, event));

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


//	#pragma mark - private kernel exported API


static select_sync_pool_entry *
find_select_sync_pool_entry(select_sync_pool *pool, selectsync *sync,
	uint32 ref)
{
	for (SelectSyncPoolEntryList::Iterator it = pool->entries.GetIterator();
		 it.HasNext();) {
		select_sync_pool_entry *entry = it.Next();
		if (entry->sync == sync && entry->ref == ref)
			return entry;
	}

	return NULL;
}


static status_t
add_select_sync_pool_entry(select_sync_pool *pool, selectsync *sync,
	uint32 ref, uint8 event)
{
	// check, whether the entry does already exist
	select_sync_pool_entry *entry = find_select_sync_pool_entry(pool, sync,
		ref);
	if (!entry) {
		entry = new (std::nothrow) select_sync_pool_entry;
		if (!entry)
			return B_NO_MEMORY;

		entry->sync = sync;
		entry->ref = ref;
		entry->events = 0;

		pool->entries.Add(entry);
	}

	entry->events |= SELECT_FLAG(event);

	return B_OK;
}


status_t
add_select_sync_pool_entry(select_sync_pool **_pool, selectsync *sync,
	uint32 ref, uint8 event)
{
	// create the pool, if necessary
	select_sync_pool *pool = *_pool;
	if (!pool) {
		pool = new (std::nothrow) select_sync_pool;
		if (!pool)
			return B_NO_MEMORY;

		*_pool = pool;
	}

	// add the entry
	status_t error = add_select_sync_pool_entry(pool, sync, ref, event);

	// cleanup
	if (pool->entries.IsEmpty()) {
		delete pool;
		*_pool = NULL;
	}

	return error;
}


status_t
remove_select_sync_pool_entry(select_sync_pool **_pool, selectsync *sync,
	uint8 event)
{
	select_sync_pool *pool = *_pool;
	if (!pool)
		return B_ENTRY_NOT_FOUND;

	// clear the event flag of the concerned entries
	bool found = false;
	for (SelectSyncPoolEntryList::Iterator it = pool->entries.GetIterator();
		 it.HasNext();) {
		select_sync_pool_entry *entry = it.Next();
		if (entry->sync == sync) {
			found = true;
			entry->events &= ~SELECT_FLAG(event);

			// remove the entry, if no longer needed
			if (entry->events == 0) {
				it.Remove();
				delete entry;
			}
		}
	}

	if (!found)
		return B_ENTRY_NOT_FOUND;

	// delete the pool, if no longer needed
	if (pool->entries.IsEmpty()) {
		delete pool;
		*_pool = NULL;
	}

	return B_OK;
}


void
delete_select_sync_pool(select_sync_pool *pool)
{
	if (!pool)
		return;

	while (select_sync_pool_entry *entry = pool->entries.Head()) {
		pool->entries.Remove(entry);
		delete entry;
	}

	delete pool;
}


void
notify_select_event_pool(select_sync_pool *pool, uint8 event)
{
	if (!pool)
		return;

	FUNCTION(("notify_select_event_pool(%p, %u)\n", pool, event));

	for (SelectSyncPoolEntryList::Iterator it = pool->entries.GetIterator();
		 it.HasNext();) {
		select_sync_pool_entry *entry = it.Next();
		if (entry->events & SELECT_FLAG(event))
			notify_select_event(entry->sync, entry->ref, event);
	}
}


//	#pragma mark - Kernel POSIX layer


ssize_t
_kern_select(int numFDs, fd_set *readSet, fd_set *writeSet, fd_set *errorSet,
	bigtime_t timeout, const sigset_t *sigMask)
{
	return common_select(numFDs, readSet, writeSet, errorSet, timeout,
		sigMask, true);
}


ssize_t
_kern_poll(struct pollfd *fds, int numFDs, bigtime_t timeout)
{
	return common_poll(fds, numFDs, timeout, true);
}


//	#pragma mark - User syscalls


ssize_t
_user_select(int numFDs, fd_set *userReadSet, fd_set *userWriteSet,
	fd_set *userErrorSet, bigtime_t timeout, const sigset_t *userSigMask)
{
	fd_set *readSet = NULL, *writeSet = NULL, *errorSet = NULL;
	uint32 bytes = _howmany(numFDs, NFDBITS) * sizeof(fd_mask);
	sigset_t sigMask;
	int result;

	if (numFDs < 0)
		return B_BAD_VALUE;

	if ((userReadSet != NULL && !IS_USER_ADDRESS(userReadSet))
		|| (userWriteSet != NULL && !IS_USER_ADDRESS(userWriteSet))
		|| (userErrorSet != NULL && !IS_USER_ADDRESS(userErrorSet))
		|| (userSigMask != NULL && !IS_USER_ADDRESS(userSigMask)))
		return B_BAD_ADDRESS;

	// copy parameters

	if (userReadSet != NULL) {
		readSet = (fd_set *)malloc(bytes);
		if (readSet == NULL)
			return B_NO_MEMORY;

		if (user_memcpy(readSet, userReadSet, bytes) < B_OK) {
			result = B_BAD_ADDRESS;
			goto err;
		}
	}

	if (userWriteSet != NULL) {
		writeSet = (fd_set *)malloc(bytes);
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
		errorSet = (fd_set *)malloc(bytes);
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

	result = common_select(numFDs, readSet, writeSet, errorSet, timeout,
		userSigMask ? &sigMask : NULL, false);

	// copy back results

	if (result >= B_OK
		&& ((readSet != NULL
				&& user_memcpy(userReadSet, readSet, bytes) < B_OK)
			|| (writeSet != NULL
				&& user_memcpy(userWriteSet, writeSet, bytes) < B_OK)
			|| (errorSet != NULL
				&& user_memcpy(userErrorSet, errorSet, bytes) < B_OK)))
		result = B_BAD_ADDRESS;

err:
	free(readSet);
	free(writeSet);
	free(errorSet);

	return result;
}


ssize_t
_user_poll(struct pollfd *userfds, int numFDs, bigtime_t timeout)
{
	struct pollfd *fds;
	size_t bytes;
	int result;

	if (numFDs < 0)
		return B_BAD_VALUE;

	if (userfds == NULL || !IS_USER_ADDRESS(userfds))
		return B_BAD_ADDRESS;

	// copy parameters

	fds = (struct pollfd *)malloc(bytes = numFDs * sizeof(struct pollfd));
	if (fds == NULL)
		return B_NO_MEMORY;

	if (user_memcpy(fds, userfds, bytes) < B_OK) {
		result = B_BAD_ADDRESS;
		goto err;
	}

	result = common_poll(fds, numFDs, timeout, false);

	// copy back results
	if (result >= B_OK && user_memcpy(userfds, fds, bytes) < B_OK)
		result = B_BAD_ADDRESS;

err:
	free(fds);

	return result;
}

