/*

	PriorityMenu.cpp

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

#include "PriorityMenu.h"
#include "PCView.h"
#include <Window.h>
#include <MenuItem.h>
#include <stdio.h>

PriorityMenu::PriorityMenu (thread_id thread, int32 priority) :
	BMenu (B_EMPTY_STRING), fThreadID (thread), fPriority (priority)
{
}

void PriorityMenu::Update (int32 priority)
{
	if (priority != fPriority && CountItems () > 0)
		RemoveItems (0, CountItems (), true);
	if (CountItems () < 1)
		BuildMenu ();
	fPriority = priority;
}

typedef struct {
	char	name[32];
	long	priority;
} PriorityRec;

static PriorityRec	priorities[] = {
	{"Idle", 	0},
	{"Lowest Active",	1},
	{"Low",	5},
	{"Normal", 10},
	{"Display", 15},
	{"Urgent Display", 20},
	{"Real Time Display", 100},
	{"Urgent", 110},
	{"Real Time", 120},
	{"",	-1}
};

PriorityRec customPriority = { "Custom", 0 };

// --------------------------------------------------------------
void PriorityMenu::BuildMenu ()
{
	BMenuItem* item;
	BMessage* message;
	char	name[B_OS_NAME_LENGTH + 20];
	long	found = false;

	for (long index = 0; ; index++) {
		PriorityRec	*priority = &priorities[index];
		if (priority->priority < 0)
			break;
		if (!found && fPriority < priority->priority) {
			priority = &customPriority;
			priority->priority = fPriority;
			index--;
		}
		message = new BMessage('PrTh');
		message->AddInt32("thread", fThreadID);
		message->AddInt32("priority", priority->priority);
		sprintf(name, "%s Priority [%d]", priority->name, (int) priority->priority);
		item = new BMenuItem(name, message);
		item->SetTarget(gPCView);
		if (fPriority == priority->priority)
			found = true, item->SetMarked(true);
		AddItem(item);
	}
}
