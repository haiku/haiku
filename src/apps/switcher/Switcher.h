/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SWITCHER_H
#define SWITCHER_H


#include <Application.h>


class BMessage;


enum {
	kNowhere			= 0,
	kTopEdge			= 0x01,
	kBottomEdge			= 0x02,
	kLeftEdge			= 0x04,
	kRightEdge			= 0x08,
	// TODO: not yet supported
	kTopLeftCorner		= 0x10,
	kTopRightCorner		= 0x20,
	kBottomLeftCorner	= 0x40,
	kBottomRightCorner	= 0x80
};

enum {
	kShowApplications,
	kShowApplicationWindows,
	kShowWorkspaceWindows,
	kShowAllWindows,
	kShowWorkspaces,
	kShowShelf,
	kShowFavorites,
	kShowRecentFiles,
	kShowFilesClipboard,
	kShowFolder,
	kShowQuery
};


static const uint32 kMsgLocationTrigger = 'LoTr';
static const uint32 kMsgLocationFree = 'LoFr';
static const uint32 kMsgHideWhenMouseMovedOut = 'HwMo';


class Switcher : public BApplication {
public:
							Switcher();
	virtual					~Switcher();

	virtual	void			ReadyToRun();

	virtual	void			MessageReceived(BMessage* message);

private:
			BMessenger		fCaptureMessenger;
			uint32			fOccupiedLocations;
};


extern const char* kSignature;


#endif	// SWITCHER_H
