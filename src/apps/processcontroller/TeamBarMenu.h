/*
	ProcessController © 2000, Georges-Edouard Berenger, All Rights Reserved.
	Copyright (C) 2004 beunited.org 

	This library is free software; you can redistribute it and/or 
	modify it under the terms of the GNU Lesser General Public 
	License as published by the Free Software Foundation; either 
	version 2.1 of the License, or (at your option) any later version. 

	This library is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	Lesser General Public License for more details. 

	You should have received a copy of the GNU Lesser General Public 
	License along with this library; if not, write to the Free Software 
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA	
*/
#ifndef _TEAM_BAR_MENU_H_
#define _TEAM_BAR_MENU_H_


#include "Utilities.h"

#include <Menu.h>


class TeamBarMenuItem;

typedef struct {
	TeamBarMenuItem*	item;
	int					index;
} TRecycleItem;


class TeamBarMenu : public BMenu {
	public:
						TeamBarMenu(const char* title, info_pack* infos, int32 teamCount);
		virtual			~TeamBarMenu();

		virtual	void	Draw(BRect updateRect);
		virtual	void	Pulse();

		team_id*		fTeamList;
		int				fTeamCount;
		TRecycleItem*	fRecycleList;
		int				fRecycleCount;
		bigtime_t		fLastTotalTime;
		bool			fFirstShow;
};

#endif // _TEAM_BAR_MENU_H_
