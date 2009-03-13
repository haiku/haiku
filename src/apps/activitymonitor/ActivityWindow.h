/*
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ACTIVITY_WINDOW_H
#define ACTIVITY_WINDOW_H


#include <Messenger.h>
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

			int32		ActivityViewCount() const;
			ActivityView* ActivityViewAt(int32 index) const;
			void		BroadcastToActivityViews(BMessage* message,
							BView* exceptToView = NULL);

			bigtime_t	RefreshInterval() const;

private:
			status_t	_OpenSettings(BFile& file, uint32 mode);
			status_t	_LoadSettings(BMessage& settings);
			status_t	_SaveSettings();

			void		_AddDefaultView();
			void		_MessageDropped(BMessage *message);

#ifdef __HAIKU__
	BGroupLayout*	fLayout;
#endif
	BMessenger		fSettingsWindow;
};

static const uint32 kMsgRemoveView = 'rmvw';

#endif	// ACTIVITY_WINDOW_H
