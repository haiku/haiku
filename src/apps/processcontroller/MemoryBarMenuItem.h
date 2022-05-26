/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEMORY_BAR_MENU_ITEM_H_
#define _MEMORY_BAR_MENU_ITEM_H_


#include <MenuItem.h>

class BBitmap;


class MemoryBarMenuItem : public BMenuItem {
	public:
						MemoryBarMenuItem(const char *label, team_id team,
							BBitmap* icon, bool deleteIcon, BMessage* message);
		virtual			~MemoryBarMenuItem();

		virtual	void	DrawContent();
		virtual	void	GetContentSize(float* _width, float* _height);

		void			DrawIcon();
		void			DrawBar(bool force);
		int				UpdateSituation(int64 committedMemory);
		void			BarUpdate();
		void			Init();
		void			Reset(char* name, team_id team, BBitmap* icon, bool deleteIcon);

	private:
		int64			fPhysicalMemory;
		int64			fCommittedMemory;
		int64			fWriteMemory;
		int64			fAllMemory;
		int64			fLastCommitted;
		int64			fLastWrite;
		int64			fLastAll;
		team_id			fTeamID;
		BBitmap*		fIcon;
		double			fGrenze1;
		double			fGrenze2;
		bool			fDeleteIcon;
};

#endif // _MEMORY_BAR_MENU_ITEM_H_
