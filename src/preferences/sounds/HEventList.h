// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        HEventList.h
//  Author:      Jérôme Duval, Oliver Ruiz Dorantes, Atsushi Takamatsu
//  Description: Sounds Preferences
//  Created :    November 24, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef __HEVENTLIST_H__
#define __HEVENTLIST_H__

#include <ListView.h>
#include "HEventItem.h"

enum{
	M_EVENT_CHANGED = 'SCAG'
};

class HEventList : public BListView {
public:
					HEventList(BRect rect
						, const char* name="EventList");
		virtual			~HEventList();
				void	RemoveAll();
				void	SetType(const char* type);
				void	SetPath(const char* path);
protected:
		virtual void	SelectionChanged();
private:
		char			*fType;	
};
#endif
