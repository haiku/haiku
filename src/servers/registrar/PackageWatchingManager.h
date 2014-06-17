/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_WATCHING_MANAGER_H
#define PACKAGE_WATCHING_MANAGER_H


#include "WatchingService.h"


class PackageWatchingManager {
public:
								PackageWatchingManager();
								~PackageWatchingManager();

			void				HandleStartStopWatching(BMessage* request);
			void				NotifyWatchers(BMessage* message);

private:
			status_t			_AddWatcher(const BMessage* request);
			status_t			_RemoveWatcher(const BMessage* request);

private:
			WatchingService		fWatchingService;
};


#endif	// PACKAGE_WATCHING_MANAGER_H
