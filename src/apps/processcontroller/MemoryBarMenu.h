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
#ifndef MEMORY_BAR_MENU_H
#define MEMORY_BAR_MENU_H


#include "Utilities.h"

#include <Menu.h>


class MemoryBarMenuItem;


typedef struct {
	MemoryBarMenuItem*	item;
	int					index;
} MRecycleItem;


class MemoryBarMenu : public BMenu {
	public:
		MemoryBarMenu(const char* name, info_pack* infos, system_info& systemInfo);
		virtual	~MemoryBarMenu();

		virtual	void	Draw(BRect updateRect);
		virtual	void	Pulse();

	private:
		team_id*		fTeamList;
		int				fTeamCount;
		MRecycleItem*	fRecycleList;
		int				fRecycleCount;
		bigtime_t		fLastTotalTime;
		bool			fFirstShow;
};

extern float gMemoryTextWidth;

#endif // MEMORY_BAR_MENU_H
