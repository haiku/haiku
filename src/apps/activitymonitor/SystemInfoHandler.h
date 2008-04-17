/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYSTEM_INFO_HANDLER_H
#define SYSTEM_INFO_HANDLER_H


#include <OS.h>
#include <Handler.h>


class SystemInfoHandler : public BHandler {
public:
						SystemInfoHandler();
						//SystemInfoHandler(BMessage *data);
	virtual				~SystemInfoHandler();

	virtual	status_t	Archive(BMessage *data, bool deep = true) const;

			void		StartWatchingStuff();
			void		StopWatchingStuff();

			void		MessageReceived(BMessage *message);

			uint32		RunningApps() const;

private:

	uint32				fRunningApps;
};

#endif	// SYSTEM_INFO_HANDLER_H
