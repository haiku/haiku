/*
 * Copyright 2008, François Revol, revol@free.fr. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYSTEM_INFO_HANDLER_H
#define SYSTEM_INFO_HANDLER_H


#include <Handler.h>


class SystemInfoHandler : public BHandler {
public:
						SystemInfoHandler();
	virtual				~SystemInfoHandler();

	virtual	status_t	Archive(BMessage* data, bool deep = true) const;

			void		StartWatching();
			void		StopWatching();

			void		MessageReceived(BMessage* message);

			uint32		RunningApps() const;
			uint32		ClipboardSize() const;
			uint32		ClipboardTextSize() const;
			uint32		MediaNodes() const;
			uint32		MediaConnections() const;
			uint32		MediaBuffers() const;

private:
			void		_UpdateClipboardData();

	uint32				fRunningApps;
	uint32				fClipboardSize;
	uint32				fClipboardTextSize;
	uint32				fMediaNodes;
	uint32				fMediaConnections;
	uint32				fMediaBuffers;
};

#endif	// SYSTEM_INFO_HANDLER_H
