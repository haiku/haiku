/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_USERLAND_FS_LOCKER_H
#define	_USERLAND_FS_LOCKER_H

#ifdef _KERNEL_MODE
	// kernel: use the RecursiveLock class


#include "RecursiveLock.h"


namespace UserlandFSUtil {
	typedef RecursiveLock Locker;
};


#else
	// userland: use the BLocker class

#include <Locker.h>

namespace UserlandFSUtil {
	typedef BLocker Locker;
};

#endif	// kernel/userland

using UserlandFSUtil::Locker;

#endif	// _USERLAND_FS_LOCKER_H
