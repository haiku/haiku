/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_SPINLOCK_CONTENTION_H
#define _SYSTEM_SPINLOCK_CONTENTION_H

#include <OS.h>


#define SPINLOCK_CONTENTION				"spinlock contention"
#define GET_SPINLOCK_CONTENTION_INFO	0x01


typedef struct spinlock_contention_info {
	bigtime_t	thread_creation_spinlock;
} spinlock_contention_info;


#endif	/* _SYSTEM_SPINLOCK_CONTENTION_H */
