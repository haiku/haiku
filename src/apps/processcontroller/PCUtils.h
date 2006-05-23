/*

	PCUtils.h

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

#ifndef _PCUTILS_H_
#define _PCUTILS_H_

#include <OS.h>
#include <GraphicsDefs.h>

class BDeskbar;
class BBitmap;
struct entry_ref;

typedef struct {
	team_info		tminfo;
	BBitmap			*tmicon;
	char			tmname[B_PATH_NAME_LENGTH];
	thread_info		*thinfo;
} infosPack;

bool get_team_name_and_icon(infosPack &infopack, bool icon = false);
bool launch (const char* mime, const char* path);
void mix_colors (rgb_color &target, rgb_color & first, rgb_color & second, float mix);
void find_self (entry_ref & ref);
void move_to_deskbar (BDeskbar & db);

extern const uchar k_cpu_mini[];
extern const uchar k_app_mini[];

#endif // _PCUTILS_H_
