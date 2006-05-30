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
#ifndef _THREAD_BAR_MENU_H_
#define _THREAD_BAR_MENU_H_


#include <Menu.h>


typedef struct {
	thread_id	thread;
	int			last_round;
} ThreadRec;

class ThreadBarMenu : public BMenu {
	public:
						ThreadBarMenu(const char *title, team_id team, int32 threadCount);
		virtual			~ThreadBarMenu();
		virtual	void	AttachedToWindow();
		virtual void	Draw(BRect updateRect);
		void			AddNew();
		void			Update();
		void			Init();
		void			Reset(team_id team);

	private:
		ThreadRec*		fThreadsRec;
		int				fThreadsRecCount;
		team_id			fTeam;
		int				fRound;
};

#endif // _THREAD_BAR_MENU_H_
