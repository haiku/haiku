/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
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
