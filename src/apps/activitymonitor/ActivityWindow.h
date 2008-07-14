/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ACTIVITY_WINDOW_H
#define ACTIVITY_WINDOW_H


#include <Window.h>

class BFile;
class BGroupLayout;
class BMenuItem;
class ActivityView;


class ActivityWindow : public BWindow {
public:
						ActivityWindow();
	virtual				~ActivityWindow();

	virtual void		MessageReceived(BMessage* message);
	virtual bool		QuitRequested();

			int32		ActivityViewCount();

private:
			status_t	_OpenSettings(BFile& file, uint32 mode);
			status_t	_LoadSettings(BMessage& settings);
			status_t	_SaveSettings();

			void		_AddDefaultView();
			void		_UpdateRemoveItem();
			void		_MessageDropped(BMessage *message);

#ifdef __HAIKU__
	BGroupLayout*	fLayout;
#endif
};

static const uint32 kMsgRemoveView = 'rmvw';

#endif	// ACTIVITY_WINDOW_H
