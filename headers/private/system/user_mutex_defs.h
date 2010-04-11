/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_USER_MUTEX_DEFS_H
#define _SYSTEM_USER_MUTEX_DEFS_H


// user mutex specific flags passed to _kern_user_mutex_unlock()
#define B_USER_MUTEX_UNBLOCK_ALL	0x80000000
	// All threads currently waiting on the mutex will be unblocked. The mutex
	// state will be locked.


// mutex value flags
#define B_USER_MUTEX_LOCKED		0x01
#define B_USER_MUTEX_WAITING	0x02
#define B_USER_MUTEX_DISABLED	0x04


#endif	/* _SYSTEM_USER_MUTEX_DEFS_H */
