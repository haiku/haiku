/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ADDON_ITEM_H_
#define _ADDON_ITEM_H_

#include <ListItem.h>
#include <String.h>
#include "ViewItem.h"
class ThemeInterfaceView;
class BCheckBox;
class BButton;

class ThemeAddonItem : public ViewItem
{
public:
					ThemeAddonItem(BRect bounds, ThemeInterfaceView *iview, int32 id);
					~ThemeAddonItem();
		void		DrawItem(BView *ownerview, BRect frame, bool complete = false);
		void		AttachedToWindow();
		void		MessageReceived(BMessage *message);
		void		Draw(BRect updateRect);
		
		void		RelocalizeStrings();
		void		RelayoutButtons();
		int32		AddonId();
		
private:
		int32		fId;
		ThemeInterfaceView	*fIView;
		BString		fAddonName;
		BCheckBox	*fApplyBox;
		BCheckBox	*fSaveBox;
		BButton		*fPrefsBtn;
};

#define CB_APPLY 'AiAp'
#define CB_SAVE 'AiSa'
#define BTN_PREFS 'AiPr'

#endif // _ADDON_ITEM_H_
