/*

	MemorySpyWindow.h

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

#ifndef _MEMORY_SPY_WINDOW_H_
#define _MEMORY_SPY_WINDOW_H_

#include <Window.h>

class MemorySpyWindow : public BWindow {

public:
				MemorySpyWindow (team_id team); 
	bool		QuitRequested ();

private:
	system_info fSysInfos;
	app_info 	fTeamInfo;
	BBitmap*	fBitmap;
};

#endif // _MEMORY_SPY_WINDOW_H_
