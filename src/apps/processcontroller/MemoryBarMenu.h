/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
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
		unsigned int	fTeamCount;
		MRecycleItem*	fRecycleList;
		int				fRecycleCount;
		bigtime_t		fLastTotalTime;
		bool			fFirstShow;
};

extern float gMemoryTextWidth;

#endif // MEMORY_BAR_MENU_H
