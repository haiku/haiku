/*
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include <View.h>
#include <Button.h>
#include <MenuField.h>
#include <Menu.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

#include <stdlib.h>

#include "AppearPrefView.h"
#include "MenuUtil.h"
#include "PrefHandler.h"
#include "TermWindow.h"
#include "TermConst.h"

BPopUpMenu *MakeFontMenu(ulong msg, const char *defaultFontName);
BPopUpMenu *MakeSizeMenu(ulong msg, uint8 defaultSize);

AppearancePrefView::AppearancePrefView (BRect frame, const char *name, TermWindow *window)
  :PrefView (frame, name)
{
  	const char *color_tbl[]=
  	{
    	PREF_TEXT_FORE_COLOR,
    	PREF_TEXT_BACK_COLOR,
    	PREF_CURSOR_FORE_COLOR,
    	PREF_CURSOR_BACK_COLOR,
    	PREF_SELECT_FORE_COLOR,
    	PREF_SELECT_BACK_COLOR,
    	NULL
  	};

 	fTermWindow = window;
	
	float fontDividerSize = StringWidth("Half Size Font") + 15.0;
	float sizeDividerSize = StringWidth("Size") + 15.0;
	
	BRect r(5,5,225,25);
	
	BMenu *menu = MakeFontMenu(MSG_HALF_FONT_CHANGED, gTermPref->getString(PREF_HALF_FONT_FAMILY));
	fHalfFont = new BMenuField(r, "halffont", "Half Width Font", menu, B_WILL_DRAW); 
	AddChild(fHalfFont);
	fHalfFont->SetDivider(fontDividerSize);
	
	r.OffsetBy(r.Width() + 10, 0);
	menu = MakeSizeMenu(MSG_HALF_SIZE_CHANGED, 12);
	fHalfFontSize = new BMenuField(r, "halfsize", "Size", menu,	B_WILL_DRAW);
	fHalfFontSize->SetDivider(sizeDividerSize);
	AddChild(fHalfFontSize);
	
	r.OffsetBy(-r.Width() - 10,r.Height() + 5); 
	menu = MakeFontMenu(MSG_FULL_FONT_CHANGED, gTermPref->getString(PREF_FULL_FONT_FAMILY));
	fFullFont = new BMenuField(r, "fullfont", "Full Width Font", menu, B_WILL_DRAW); 
	AddChild(fFullFont);
	fFullFont->SetDivider(fontDividerSize);
  
	r.OffsetBy(r.Width() + 10, 0);
	menu = MakeSizeMenu(MSG_FULL_SIZE_CHANGED, 12);
	fFullFontSize = new BMenuField(r, "fullsize", "Size", menu,	B_WILL_DRAW);
	fFullFontSize->SetDivider(sizeDividerSize);
	AddChild(fFullFontSize);  
	
	r.OffsetBy(-r.Width() - 10,r.Height() + 25); 
	fColorField = new BMenuField(r, NULL, NULL, MakeMenu (MSG_COLOR_FIELD_CHANGED, color_tbl, color_tbl[0]), B_WILL_DRAW);
	AddChild(fColorField);		

  	fColorCtl = SetupBColorControl(BPoint(r.left, r.bottom + 10), B_CELLS_32x8, 6, 
  									MSG_COLOR_CHANGED);
  	fColorCtl->SetValue(gTermPref->getRGB(PREF_TEXT_FORE_COLOR));
}

void
AppearancePrefView::Revert (void)
{
  	fColorField->Menu()->ItemAt(0)->SetMarked (true);
  	fColorCtl->SetValue (gTermPref->getRGB(PREF_TEXT_FORE_COLOR));
	
  	fHalfFont->Menu()->FindItem (gTermPref->getString(PREF_HALF_FONT_FAMILY))->SetMarked(true);
  	fFullFont->Menu()->FindItem (gTermPref->getString(PREF_FULL_FONT_FAMILY))->SetMarked(true);
  	fHalfFontSize->Menu()->FindItem (gTermPref->getString(PREF_HALF_FONT_FAMILY))->SetMarked(true);
  	fFullFontSize->Menu()->FindItem (gTermPref->getString(PREF_FULL_FONT_FAMILY))->SetMarked(true);
}

void
AppearancePrefView::AttachedToWindow (void)
{
  	fHalfFontSize->Menu()->SetTargetForItems(this);
  	fFullFontSize->Menu()->SetTargetForItems(this);  
  	
  	fHalfFont->Menu()->SetTargetForItems(this);
  	fFullFont->Menu()->SetTargetForItems(this);  

  	fColorCtl->SetTarget (this);
  	fColorField->Menu()->SetTargetForItems(this);
}

void
AppearancePrefView::MessageReceived (BMessage *msg)
{
  	bool modified = false;
  
  	switch (msg->what)
  	{ 
		case MSG_HALF_FONT_CHANGED:
		{ 
			gTermPref->setString (PREF_HALF_FONT_FAMILY, fHalfFont->Menu()->FindMarked()->Label());
			modified = true;
			break;
		}
		case MSG_HALF_SIZE_CHANGED:
	  	{
			gTermPref->setString (PREF_HALF_FONT_SIZE, fHalfFontSize->Menu()->FindMarked()->Label());
			modified = true;
			break;
		}
		case MSG_FULL_FONT_CHANGED:
	  	{ 
			gTermPref->setString (PREF_FULL_FONT_FAMILY, fFullFont->Menu()->FindMarked()->Label());
			modified = true;
			break;
		}
		case MSG_FULL_SIZE_CHANGED:
	  	{ 
			gTermPref->setString (PREF_FULL_FONT_SIZE, fFullFontSize->Menu()->FindMarked()->Label());
			modified = true;
			break;
		}
		case MSG_COLOR_CHANGED:
	  	{ 
			gTermPref->setRGB(fColorField->Menu()->FindMarked()->Label(), fColorCtl->ValueAsColor());    
			modified = true;
			break;
		}
		case MSG_COLOR_FIELD_CHANGED:
	  	{ 
			fColorCtl->SetValue(gTermPref->getRGB (fColorField->Menu()->FindMarked()->Label()));
			break;
		}
		default:
	  	{ 
			PrefView::MessageReceived(msg);
			return; // Oh !      
		}
	}
	
  	if(modified)
  	{
   		fTermWindow->PostMessage(msg);
   		Window()->PostMessage(MSG_PREF_MODIFIED);
	}
}

BPopUpMenu *
MakeFontMenu(ulong msg, const char *defaultFontName)
{
	BPopUpMenu *menu = new BPopUpMenu("");
	int32 numFamilies = count_font_families(); 
	
	for ( int32 i = 0; i < numFamilies; i++ )
	{
		font_family family; 
		uint32 flags; 
		
		if ( get_font_family(i, &family, &flags) == B_OK )
		{ 
			menu->AddItem(new BMenuItem(family, new BMessage(msg)));
			if(!strcmp(defaultFontName, family))
			{
				BMenuItem *item=menu->ItemAt(i);
				item->SetMarked(true);
			}
		}
	}
	
	return menu;
}

BPopUpMenu *
MakeSizeMenu(ulong msg, uint8 defaultSize)
{
	BPopUpMenu *menu = new BPopUpMenu("size");
	
	BMenuItem *item;
	
	for(uint8 i=9; i<13; i++)
	{
		BString string;
		string << i;
		
		item = new BMenuItem(string.String(), new BMessage(msg));
		menu->AddItem(item);
		
		if(i == defaultSize)
			item->SetMarked(true);
			
	}
	
	item = new BMenuItem("14", new BMessage(msg));
	menu->AddItem(item);
	
	if(defaultSize == 14)
		item->SetMarked(true);
	
	item = new BMenuItem("16", new BMessage(msg));
	menu->AddItem(item);
	
	if(defaultSize == 16)
		item->SetMarked(true);
	
	return menu;
}
