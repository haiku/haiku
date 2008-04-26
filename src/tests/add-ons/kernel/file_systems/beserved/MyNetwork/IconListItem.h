//------------------------------------------------------------------------------
// IconListItem.h
//------------------------------------------------------------------------------
// A ListItem implementation that displays an icon and its label.
//
// IconListItem implementation Copyright (C) 1999 Fabien Fulhaber <fulhaber@evhr.net>
// Special thanks to Brendan Allen <darkmoon96@tcon.net> for his help.
// Thanks to NPC community (http://www.beroute.tzo.com/npc/).
// This code is free to use in any way so long as the credits above remain intact.
// This code carries no warranties or guarantees of any kind. Use at your own risk.
//------------------------------------------------------------------------------

#ifndef _ICON_LIST_ITEM_H_
#define _ICON_LIST_ITEM_H_

//------------------------------------------------------------------------------
// I N C L U D E S
//------------------------------------------------------------------------------

#include <Window.h>
#include <View.h>
#include <ListView.h>
#include <ListItem.h>
#include <Bitmap.h>
#include <Rect.h>
#include <Font.h>
#include <String.h>

//------------------------------------------------------------------------------
// D E C L A R A T I O N S
//------------------------------------------------------------------------------

const int32 MSG_LIST_DESELECT = 'CLV0';

class IconListView : public BListView
{
	public:
		IconListView(BRect frame, const char *name,
			list_view_type type = B_SINGLE_SELECTION_LIST,
			uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			uint32 flags = B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS)
			: BListView(frame, name, type, resizingMode, flags)
		{
		}

		~IconListView()
		{
		}

		void MouseDown(BPoint point)
		{
			BListView::MouseDown(point);
			if (CurrentSelection() < 0)
				Window()->PostMessage(MSG_LIST_DESELECT);
		}
};

class IconListItem : public BListItem
{ 
	public:
		IconListItem(BBitmap *mini_icon, char *text, uint32 data, int level, bool expanded); 
		const char *GetItemText()		{ return label.String(); }
		const uint32 GetItemData()		{ return extra; }

		virtual ~IconListItem(); 
		virtual void DrawItem(BView *owner, BRect frame, bool complete = false); 
		virtual void Update(BView *owner, const BFont *font);

	private:
		BRect bounds; 
		BBitmap *icon;
		BString label;
		uint32 extra;
};

#endif
//------------------------------------------------------------ IconListMenu.h --
