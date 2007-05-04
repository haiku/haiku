/*
 * Copyright 2007, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PATH_MONITOR_H
#define _PATH_MONITOR_H


#include <NodeMonitor.h>


// additional flags (combined with those in NodeMonitor.h)
#define	B_WATCH_FILES_ONLY	0x0100
#define B_WATCH_RECURSIVELY	0x0200

#define B_PATH_MONITOR		'_PMN'

namespace BPrivate {

class PathHandler;

class BPathMonitor {
	public:
		BPathMonitor();
		BPathMonitor(const char* path, uint32 flags, BMessenger target);
		~BPathMonitor();

		status_t InitCheck() const;
		status_t SetTo(const char* path, uint32 flags, BMessenger target);
		status_t SetTarget(BMessenger target);
		void Unset();

	private:
		PathHandler*	fHandler;
		status_t		fStatus;
};

}	// namespace BPrivate

#endif	// _PATH_MONITOR_H
