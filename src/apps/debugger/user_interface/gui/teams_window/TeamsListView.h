/*
 * Copyright 2009-2010, Philippe Houdoin, phoudoin@haiku-os.org. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAMS_LIST_ITEM_H
#define TEAMS_LIST_ITEM_H


#include <ListView.h>
#include <StringItem.h>
#include <OS.h>

class BBitmap;
class BMessageRunner;


class TeamListItem : public BStringItem {
public:
								TeamListItem(team_info & teamInfo);
								TeamListItem(team_id teamId);
	virtual 					~TeamListItem();

public:
	virtual void 				DrawItem(BView *owner, BRect itemRect,
									bool drawEverything = false);
	virtual void 				Update(BView *owner, const BFont *font);

			team_id				TeamID()  { return fTeamInfo.team; };

	static 	int 				Compare(const void* a, const void* b);

private:
			status_t			_SetTo(team_info & info);

private:
			team_info			fTeamInfo;
			BBitmap *			fIcon;
			float				fBaselineOffset;
};


class TeamsListView : public BListView {
public:
								TeamsListView(BRect frame, const char* name);
	virtual 					~TeamsListView();

			TeamListItem* 		FindItem(team_id teamId);

protected:
	virtual void 				AttachedToWindow();
	virtual void 				DetachedFromWindow();

	virtual void 				MessageReceived(BMessage* message);

private:
			void 				_InitList();
			void 				_UpdateList();

private:
			BMessageRunner*		fUpdateRunner;
			team_id				fThisTeam;
};

static const uint32 kMsgUpdateTeamsList = 'uptl';

#endif	// TEAMS_LIST_VIEW_H
