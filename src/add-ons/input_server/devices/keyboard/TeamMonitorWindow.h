/*
 * Copyright 2004-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Axel Doerfler, axeld@pinc-software.de
 */
#ifndef TEAM_MONITOR_WINDOW_H
#define TEAM_MONITOR_WINDOW_H


#include <Box.h>
#include <Button.h>
#include <ListView.h>
#include <Window.h>

#include "TeamListItem.h"


class TeamDescriptionView;

class TeamMonitorWindow : public BWindow {
public:
							TeamMonitorWindow();
	virtual					~TeamMonitorWindow();

	virtual void			MessageReceived(BMessage* message);
	virtual bool			QuitRequested();

			void			Enable();
			void			Disable();

private:
			void			UpdateList();

			bool			fQuitting;
			BMessageRunner*	fUpdateRunner;
			BListView*		fListView;
			BButton*		fCancelButton;
			BButton*		fKillButton;
			BButton*		fRestartButton;
			TeamDescriptionView*		fDescriptionView;
};

static const uint32 kMsgCtrlAltDelPressed = 'TMcp';

#endif	// TEAM_MONITOR_WINDOW_H
