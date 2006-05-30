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
