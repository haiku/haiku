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
#include <HashMap.h>
#include <MessageFilter.h>
#include <OutlineListView.h>
#include <Window.h>

#include "TeamListItem.h"


class TeamDescriptionView;

class TeamMonitorWindow : public BWindow {
public:
							TeamMonitorWindow();
	virtual					~TeamMonitorWindow();

	virtual void			MessageReceived(BMessage* message);
	virtual void			Show();
	virtual bool			QuitRequested();

			void			Enable();
			void			Disable();
			void			LocaleChanged();
			void			QuitTeam(TeamListItem* item);
			void			MarkUnquittableTeam(BMessage* message);
			bool			HandleKeyDown(BMessage* msg);

private:
			void			_UpdateList();

			bool			fQuitting;
			BMessageRunner*	fUpdateRunner;
			BOutlineListView*	fListView;
			BButton*		fCancelButton;
			BButton*		fKillButton;
			BButton*		fQuitButton;
			BButton*		fRestartButton;
			TeamDescriptionView*	fDescriptionView;
			BList			fTeamQuitterList;
			HashMap<HashKey32<int32>, TeamListItem*>	fItemMap;
};

static const uint32 kMsgCtrlAltDelPressed = 'TMcp';
static const uint32 kMsgDeselectAll = 'TMds';
static const uint32 kMsgQuitFailed = 'TMqf';

#endif	// TEAM_MONITOR_WINDOW_H
