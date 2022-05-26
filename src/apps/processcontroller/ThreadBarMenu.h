/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
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
