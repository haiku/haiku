/*
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <sys/event.h>

#include <StackOrHeapArray.h>

#include <libroot/errno_private.h>
#include <syscalls.h>
#include <event_queue_defs.h>


extern "C" int
kqueue()
{
	int fd = _kern_event_queue_create(0);
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

	event_wait_info* waitInfo = waitInfos;
	int changedInfos = 0;

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

			default:
				return EINVAL;
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

		changedInfos++;
		waitInfo++;
	}
	if (changedInfos != 0) {
		status_t status = _kern_event_queue_select(kq, waitInfos, changedInfos);
		if (status != B_OK) {
			if (nchanges == 1 && nevents == 0) {
				// Special case: return the lone error directly.
				__set_errno(waitInfos[0].events);
				return -1;
			}

			// Report problems as error events.
			int errors = 0;
			for (int i = 0; i < changedInfos; i++) {
				if (waitInfos[i].events > 0)
					continue;
				if (nevents == 0)
					break;

				short filter = filter_from_info(waitInfos[i]);
				int64_t data = waitInfos[i].events;
				EV_SET(eventlist, waitInfos[i].object,
					filter, EV_ERROR, 0, data, waitInfos[i].user_data);
				eventlist++;
				nevents--;
				errors++;
			}
			if (nevents == 0 || errors == 0) {
				__set_errno(status);
				return -1;
			}
		}
	}

	if (nevents != 0) {
		bigtime_t timeout = 0;
		uint32 waitFlags = 0;
		if (tspec != NULL) {
			timeout = (tspec->tv_sec * 1000000LL) + (tspec->tv_nsec / 1000LL);
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
				} else if ((waitInfos[i].events & B_EVENT_DISCONNECTED) != 0) {
					flags |= EV_EOF;
				} else if ((waitInfos[i].events & B_EVENT_INVALID) != 0) {
					switch (waitInfos[i].type) {
						case B_OBJECT_TYPE_FD:
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
