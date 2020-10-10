/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _THREAD_BAR_MENU_ITEM_H_
#define _THREAD_BAR_MENU_ITEM_H_


#include <MenuItem.h>


class ThreadBarMenuItem : public BMenuItem {
	public:
						ThreadBarMenuItem(const char* title, thread_id thread,
							BMenu* menu, BMessage* msg);

		virtual	void	DrawContent();
		virtual	void	GetContentSize(float* width, float* height);
		virtual	void	Highlight(bool on);
		void			DrawBar(bool force);
		void			BarUpdate();

		double			fUser;
		double			fKernel;

	private:
		thread_id		fThreadID;
		thread_info		fThreadInfo;
		bigtime_t		fLastTime;
		float			fGrenze1;
		float			fGrenze2;
};

#endif // _THREAD_BAR_MENU_ITEM_H_
