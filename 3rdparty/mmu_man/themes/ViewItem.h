#ifndef _VIEW_ITEM_H_
#define _VIEW_ITEM_H_

#include <ListItem.h>
#include <ListView.h>
#include <View.h>

class ViewItem : public BView, public BListItem
{
public:
						ViewItem(BRect bounds, const char *name, uint32 resizeMask, uint32 flags, uint32 level = 0, bool expanded = true);
	virtual				~ViewItem();
	virtual void		DrawItem(BView *ownerview, BRect frame, bool complete = false);
	virtual void		Update(BView *ownerview, const BFont *font);
private:
	BListView*			fOwner;
};

#endif // _VIEW_ITEM_H_
