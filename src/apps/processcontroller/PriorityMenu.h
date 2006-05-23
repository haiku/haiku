/*

	PriorityMenu.h

	ProcessController
	© 2000, Georges-Edouard Berenger, All Rights Reserved.
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

#ifndef _PRIORITY_MENU_H_
#define _PRIORITY_MENU_H_

#include <Menu.h>

class PriorityMenu : public BMenu
{
public:
						PriorityMenu (thread_id thread, int32 priority);

		void			Update (int32 priority);
		void			BuildMenu ();

private:
		thread_id		fThreadID;
		int32			fPriority;
};


#endif // _PRIORITY_MENU_H_
