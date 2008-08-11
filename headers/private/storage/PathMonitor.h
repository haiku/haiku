/*
 * Copyright 2007-2008, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PATH_MONITOR_H
#define _PATH_MONITOR_H


#include <NodeMonitor.h>


// additional flags (combined with those in NodeMonitor.h)
#define	B_WATCH_FILES_ONLY		0x0100
#define B_WATCH_RECURSIVELY		0x0200
#define	B_WATCH_FOLDERS_ONLY	0x0400
// NOTE: B_WATCH_RECURSIVELY usually implies to watch for file changes as well,
// if that is not desired, add B_WATCH_FOLDERS_ONLY to the flags.

#define B_PATH_MONITOR		'_PMN'

namespace BPrivate {

class BPathMonitor {
public:
	static	status_t			StartWatching(const char* path, uint32 flags,
									BMessenger target);

	static	status_t			StopWatching(const char* path,
									BMessenger target);
	static	status_t			StopWatching(BMessenger target);

private:
								BPathMonitor();
								~BPathMonitor();

	static	status_t			_InitLockerIfNeeded();
	static	status_t			_InitLooperIfNeeded();
};

}	// namespace BPrivate

#endif	// _PATH_MONITOR_H
