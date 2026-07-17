/*
 * Copyright 2023-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <sys/event.h>

#include <StackOrHeapArray.h>

#include <libroot/errno_private.h>
#include <libroot/time_private.h>
#include <syscalls.h>
#include <event_queue_defs.h>


extern "C" int
kqueue()
{
	int fd = _kern_event_queue_create(O_CLOFORK);
	if (fd < 0) {
		__set_errno(fd);
		return -1;
	}
	return fd;
}


static short
filter_from_info(const event_wait_info& info)
{
	switch (info.type) {
		case B_OBJECT_TYPE_FD:
			if (info.events > 0 && (info.events & B_EVENT_WRITE) != 0)
				return EVFILT_WRITE;
			return EVFILT_READ;

		case B_OBJECT_TYPE_THREAD:
			return EVFILT_PROC;

		case B_OBJECT_TYPE_SEMAPHORE:
			return EVFILT_HAIKU_SEM;

		case B_OBJECT_TYPE_PORT:
			return EVFILT_HAIKU_PORT_READ;
	}

	return 0;
}


extern "C" int
kevent(int kq,
	const struct kevent *changelist, int nchanges,
	struct kevent *eventlist, int nevents,
	const struct timespec *tspec)
{
	BStackOrHeapArray<event_wait_info, 16> waitInfos(max_c(nchanges, nevents));
	BStackOrHeapArray<int32, 16> requestedEvents(nchanges);

	event_wait_info* waitInfo = waitInfos;
	int32* requestedEvent = requestedEvents;
	int changedInfos = 0, receiptCount = 0;

	for (int i = 0; i < nchanges; i++) {
		waitInfo->object = changelist[i].ident;
		waitInfo->events = 0;
		waitInfo->user_data = changelist[i].udata;

		int32 events = 0, behavior = 0;
		switch (changelist[i].filter) {
			case EVFILT_READ:
				waitInfo->type = B_OBJECT_TYPE_FD;
				events = B_EVENT_READ;
				break;

			case EVFILT_WRITE:
				waitInfo->type = B_OBJECT_TYPE_FD;
				events = B_EVENT_WRITE;
				break;

			case EVFILT_PROC:
				waitInfo->type = B_OBJECT_TYPE_THREAD;
				if ((changelist[i].fflags & NOTE_EXIT) != 0)
					events |= B_EVENT_INVALID;
				break;

			case EVFILT_HAIKU_SEM:
				waitInfo->type = B_OBJECT_TYPE_SEMAPHORE;
				events = B_EVENT_ACQUIRE_SEMAPHORE;
				break;

			case EVFILT_HAIKU_PORT_READ:
				waitInfo->type = B_OBJECT_TYPE_PORT;
				events = B_EVENT_READ;
				break;

			default:
				__set_errno(EINVAL);
				return -1;
		}

		if ((changelist[i].flags & EV_ONESHOT) != 0)
			behavior |= B_EVENT_ONE_SHOT;
		if ((changelist[i].flags & EV_CLEAR) == 0)
			behavior |= B_EVENT_LEVEL_TRIGGERED;

		if (changelist[i].filter == EVFILT_READ || changelist[i].filter == EVFILT_WRITE) {
			// kqueue treats the same file descriptor with both READ and WRITE filters
			// as two separate listeners. Haiku, however, treats it as one.
			// We rectify this here by carefully combining the two.

			// We can't support ONESHOT for descriptors due to the separation.
			if ((changelist[i].flags & EV_ONESHOT) != 0) {
				__set_errno(EOPNOTSUPP);
				return -1;
			}

			const short otherFilter = (changelist[i].filter == EVFILT_READ)
				? EVFILT_WRITE : EVFILT_READ;
			const int32 otherEvents = (otherFilter == EVFILT_READ)
				? B_EVENT_READ : B_EVENT_WRITE;

			// First, check if the other filter is specified in this changelist.
			int j;
			for (j = 0; j < nchanges; j++) {
				if (changelist[j].ident != changelist[i].ident)
					continue;
				if (changelist[j].filter != otherFilter)
					continue;

				// We've found it.
				break;
			}
			if (j < nchanges) {
				// It is in the list.
				if (j < i) {
					// And it's already been taken care of.
					continue;
				}

				// Fold it into this one.
				if ((changelist[j].flags & EV_ADD) != 0) {
					waitInfo->events |= otherEvents;
				} else if ((changelist[j].flags & EV_DELETE) != 0) {
					waitInfo->events &= ~otherEvents;
				}
			} else {
				// It is not in the list. See if it's already set.
				event_wait_info info;
				info.type = B_OBJECT_TYPE_FD;
				info.object = waitInfo->object;
				info.events = -1;

				status_t status = _kern_event_queue_select(kq, &info, 1);
				if (status == B_OK)
					waitInfo->events |= (info.events & otherEvents);
			}
		}

		if ((changelist[i].flags & EV_ADD) != 0) {
			waitInfo->events |= events;
		} else if ((changelist[i].flags & EV_DELETE) != 0) {
			waitInfo->events &= ~events;
		}

		if (waitInfo->events != 0)
			waitInfo->events |= behavior;

		*requestedEvent = waitInfo->events;

		if ((changelist[i].flags & EV_RECEIPT) != 0) {
			receiptCount++;

			// Use sign bit to indicate EV_RECEIPT.
			*requestedEvent |= (1 << 31);
		}

		changedInfos++;
		waitInfo++;
		requestedEvent++;

		if (receiptCount >= nevents)
			break;
	}
	if (changedInfos != 0) {
		status_t status = _kern_event_queue_select(kq, waitInfos, changedInfos);
		if (status != B_OK && nchanges == 1 && nevents == 0) {
			// Special case: return the lone error directly.
			__set_errno(waitInfos[0].events);
			return -1;
		}

		// Report problems (or successes, if EV_RECEIPT is set) as error events.
		int errors = 0;
		for (int i = 0; i < changedInfos; i++) {
			int64_t data = waitInfos[i].events;
			if (data > 0) {
				if (requestedEvents[i] > 0)
					continue;

				// Always generate an "error" event for EV_RECEIPT.
				data = 0;
			}

			if (nevents == 0) {
				errors = -1;
				break;
			}

			short filter = filter_from_info(waitInfos[i]);
			if ((requestedEvents[i] & (B_EVENT_READ | B_EVENT_WRITE))
					== (B_EVENT_READ | B_EVENT_WRITE)) {
				// We need to generate two errors for this case.
				filter = EVFILT_READ;
				int64_t readData = data;
				if (data == 0 && (waitInfos[i].events & B_EVENT_READ) == 0)
					readData = -1;

				EV_SET(eventlist, waitInfos[i].object,
					filter, EV_ERROR, 0, readData, waitInfos[i].user_data);
				eventlist++;
				nevents--;
				errors++;

				filter = EVFILT_WRITE;
				if (data == 0 && (waitInfos[i].events & B_EVENT_WRITE) == 0)
					data = -1;
			}

			if (nevents == 0) {
				errors = -1;
				break;
			}

			EV_SET(eventlist, waitInfos[i].object,
				filter, EV_ERROR, 0, data, waitInfos[i].user_data);
			eventlist++;
			nevents--;
			errors++;
		}

		if (errors > 0)
			return errors;
		if (status != B_OK) {
			__set_errno(status);
			return -1;
		}
	}

	if (nevents != 0) {
		bigtime_t timeout = 0;
		uint32 waitFlags = 0;
		if (tspec != NULL) {
			if (!timespec_to_bigtime(*tspec, timeout)) {
				__set_errno(EINVAL);
				return -1;
			}
			waitFlags |= B_RELATIVE_TIMEOUT;
		}

		ssize_t events = _kern_event_queue_wait(kq, waitInfos,
			max_c(1, nevents / 2), waitFlags, timeout);
		if (events > 0) {
			int returnedEvents = 0;
			for (ssize_t i = 0; i < events; i++) {
				unsigned short flags = 0;
				unsigned int fflags = 0;
				int64_t data = 0;

				if (waitInfos[i].events < 0) {
					flags |= EV_ERROR;
					data = waitInfos[i].events;
				} else if ((waitInfos[i].events & B_EVENT_INVALID) != 0) {
					switch (waitInfos[i].type) {
						case B_OBJECT_TYPE_FD:
							// File descriptor was closed. Silently discard the event.
							continue;

						case B_OBJECT_TYPE_SEMAPHORE:
						case B_OBJECT_TYPE_PORT:
							flags |= EV_EOF;
							break;

						case B_OBJECT_TYPE_THREAD: {
							fflags |= NOTE_EXIT;

							status_t returnValue = -1;
							status_t status = wait_for_thread(waitInfos[i].object, &returnValue);
							if (status == B_OK)
								data = returnValue;
							else
								data = -1;
							break;
						}
					}
				} else if ((waitInfos[i].events & B_EVENT_DISCONNECTED) != 0) {
					flags |= EV_EOF;
				} else if ((waitInfos[i].events & B_EVENT_ERROR) != 0) {
					flags |= EV_ERROR;
					data = EINVAL;
				}

				short filter = filter_from_info(waitInfos[i]);
				if (waitInfos[i].type == B_OBJECT_TYPE_FD && (flags & (EV_ERROR | EV_EOF)) == 0) {
					// Do we have both a read and a write event?
					if ((waitInfos[i].events & (B_EVENT_READ | B_EVENT_WRITE))
							== (B_EVENT_READ | B_EVENT_WRITE)) {
						// We do. Report both, if we can.
						if (nevents > 1) {
							EV_SET(eventlist, waitInfos[i].object,
								EVFILT_WRITE, flags, fflags, data, waitInfos[i].user_data);
							eventlist++;
							returnedEvents++;
							nevents--;
						}
						filter = EVFILT_READ;
					}
				}

				EV_SET(eventlist, waitInfos[i].object,
					filter, flags, fflags, data, waitInfos[i].user_data);
				eventlist++;
				returnedEvents++;
				nevents--;
			}
			return returnedEvents;
		} else if (events < 0) {
			if (events == B_WOULD_BLOCK || events == B_TIMED_OUT)
				return 0;

			__set_errno(events);
			return -1;
		}
		return 0;
	}

	return 0;
}
