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
#include <TextControl.h>
#include <Beep.h>

#include <stdlib.h>

#include "AppearPrefView.h"
#include "PrefHandler.h"
#include "TermWindow.h"
#include "TermConst.h"
#include "MenuUtil.h"
#include "TTextControl.h"

#define MAX_FONT_SIZE 32
#define MIN_FONT_SIZE 6

AppearancePrefView::AppearancePrefView (BRect frame, const char *name,
					TermWindow *window)
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

	mHalfFont = new BMenuField(BRect(0,0,220,20), "halffont", "Half Size Font", MakeFontMenu(MSG_HALF_FONT_CHANGED, gTermPref->getString(PREF_HALF_FONT_FAMILY)), B_WILL_DRAW); 
	AddChild(mHalfFont);
	mHalfFont->SetDivider (120);
	
	mHalfSize = new TTextControl(BRect(220, 0, 300, 20), "halfsize", "Size", "", new BMessage(MSG_HALF_SIZE_CHANGED));
	AddChild(mHalfSize);
  	mHalfSize->SetText(gTermPref->getString(PREF_HALF_FONT_SIZE));
	mHalfSize->SetDivider (50);

	mFullFont = new BMenuField(BRect(0,30,220,50), "fullfont", "Full Size Font", MakeFontMenu(MSG_FULL_FONT_CHANGED, gTermPref->getString(PREF_FULL_FONT_FAMILY)), B_WILL_DRAW); 
	AddChild(mFullFont);
	mFullFont->SetDivider (120);
  
	mFullSize = new TTextControl(BRect(220, 30, 300, 50), "fullsize", "Size", "", new BMessage(MSG_FULL_SIZE_CHANGED));
	AddChild(mFullSize);  
	mFullSize->SetText(gTermPref->getString(PREF_FULL_FONT_SIZE));
	mFullSize->SetDivider (50);

	mColorField = new BMenuField(BRect(0,60,200,80), NULL, NULL, MakeMenu (MSG_COLOR_FIELD_CHANGED, color_tbl, color_tbl[0]), B_WILL_DRAW);
	AddChild(mColorField);		

  	mColorCtl = SetupBColorControl(BPoint(20, 85), B_CELLS_32x8, 6, MSG_COLOR_CHANGED);
  	mColorCtl->SetValue(gTermPref->getRGB(PREF_TEXT_FORE_COLOR));

}

void
AppearancePrefView::Revert (void)
{

  	mHalfSize->SetText (gTermPref->getString (PREF_HALF_FONT_SIZE));
  	mFullSize->SetText (gTermPref->getString (PREF_FULL_FONT_SIZE));
  	mColorField->Menu()->ItemAt(0)->SetMarked (true);
  	mColorCtl->SetValue (gTermPref->getRGB(PREF_TEXT_FORE_COLOR));
	
  	mHalfFont->Menu()->FindItem (gTermPref->getString(PREF_HALF_FONT_FAMILY))->SetMarked(true);
  	mFullFont->Menu()->FindItem (gTermPref->getString(PREF_FULL_FONT_FAMILY))->SetMarked(true);

  	return;
  
}

////////////////////////////////////////////////////////////////////////////
// SaveIfModified (void)
//
////////////////////////////////////////////////////////////////////////////
void
AppearancePrefView::SaveIfModified (void)
{
  	BMessenger messenger (fTermWindow);
  
  	if (mHalfSize->IsModified()) {
    	gTermPref->setString (PREF_HALF_FONT_SIZE, mHalfSize->Text());
    	messenger.SendMessage (MSG_HALF_SIZE_CHANGED);
    	mHalfSize->ModifiedText (false);
  	}	
  	if (mFullSize->IsModified()) {
    	gTermPref->setString (PREF_FULL_FONT_SIZE, mFullSize->Text());
    	messenger.SendMessage (MSG_FULL_SIZE_CHANGED);
    	mFullSize->ModifiedText (false);
  	}
  	return;
}

void
AppearancePrefView::AttachedToWindow (void)
{
  	mHalfSize->SetTarget (this);
  	mFullSize->SetTarget (this);
  	mHalfFont->Menu()->SetTargetForItems(this);
  	mFullFont->Menu()->SetTargetForItems(this);  

  	mColorCtl->SetTarget (this);
  	mColorField->Menu()->SetTargetForItems(this);

}

void
AppearancePrefView::MessageReceived (BMessage *msg)
{
  	bool modified = false;
  	int size;
  
  	switch (msg->what) {  
  	case MSG_HALF_FONT_CHANGED:
    	gTermPref->setString (PREF_HALF_FONT_FAMILY, mHalfFont->Menu()->FindMarked()->Label());
    	modified = true;
    break;
      
  	case MSG_HALF_SIZE_CHANGED:
    	size = atoi (mHalfSize->Text());
    	if (size > MAX_FONT_SIZE || size < MIN_FONT_SIZE) {
     	 	mHalfSize->SetText (gTermPref->getString (PREF_HALF_FONT_SIZE));
      		beep ();
    	} else {
      		gTermPref->setString (PREF_HALF_FONT_SIZE, mHalfSize->Text());
      		modified = true;
    	}
   	break;
      
  	case MSG_FULL_FONT_CHANGED:
    	gTermPref->setString (PREF_FULL_FONT_FAMILY, mFullFont->Menu()->FindMarked()->Label());
    	modified = true;
    break;
      
 	case MSG_FULL_SIZE_CHANGED:
   		size = atoi (mHalfSize->Text());
    	if (size > MAX_FONT_SIZE || size < MIN_FONT_SIZE) {
     	mFullSize->SetText (gTermPref->getString (PREF_FULL_FONT_SIZE));
      	beep ();
    } else {
      	gTermPref->setString (PREF_FULL_FONT_SIZE, mFullSize->Text());
    }
    	modified = true;
    break;
      
  	case MSG_COLOR_CHANGED:
    	gTermPref->setRGB(mColorField->Menu()->FindMarked()->Label(), mColorCtl->ValueAsColor());    
    	modified = true;
    break;

  	case MSG_COLOR_FIELD_CHANGED:
    	mColorCtl->SetValue(gTermPref->getRGB (mColorField->Menu()->FindMarked()->Label()));
    break;
  
  	default:
    	PrefView::MessageReceived(msg);
    return; // Oh !      
  }

  	if(modified){
   		fTermWindow->PostMessage (msg);
   		(this->Window())->PostMessage(MSG_PREF_MODIFIED);
  }
}
