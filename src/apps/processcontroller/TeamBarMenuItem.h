/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TEAM_BAR_MENU_ITEM_H_
#define _TEAM_BAR_MENU_ITEM_H_


#include <MenuItem.h>

class BBitmap;


class TeamBarMenuItem : public BMenuItem {
public:
					TeamBarMenuItem(BMenu* menu, BMessage* kill_team, team_id team,
						BBitmap* icon, bool deleteIcon);

	virtual			~TeamBarMenuItem();

	virtual	void	DrawContent();
	virtual	void	GetContentSize(float* width, float* height);
	void			DrawIcon();
	void			DrawBar(bool force);
	void			BarUpdate();
	void			Init();
	void			Reset(char* name, team_id team, BBitmap* icon, bool deleteIcon);

	double			fUser;
	double			fKernel;

private:
	team_id			fTeamID;
	BBitmap*		fIcon;
	team_usage_info	fTeamUsageInfo;
	bigtime_t		fLastTime;
	float			fGrenze1;
	float			fGrenze2;
	bool			fDeleteIcon;
};


#endif // _TEAM_BAR_MENU_ITEM_H_
