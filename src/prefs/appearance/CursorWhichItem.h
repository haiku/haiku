#ifndef COLORWHICH_ITEM_H
#define COLORWHICH_ITEM_H

#include <InterfaceDefs.h>
#include <ListItem.h>

#include <SysCursor.h>

class CursorWhichItem : public BStringItem
{
public:
	CursorWhichItem(cursor_which which);
	~CursorWhichItem(void);
	void SetAttribute(cursor_which which);
	cursor_which GetAttribute(void);
private:
	cursor_which attribute;
};

#endif