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
 
#include <Application.h>
#include <StringView.h>
#include <ColorControl.h>
#include "ColorWindow.h"
#include "PrefHandler.h"
#include "PrefView.h"
#include <PopUpMenu.h>
#include <Menu.h>
#include <MenuField.h>
#include <Window.h>
#include <MenuItem.h>
#include "TermConst.h"
#include "TermWindow.h"
#include "TermView.h"
#include "TermBuffer.h"
#include "MenuUtil.h"
#include "TTextControl.h"



// Set up the window


colWindow::colWindow(const char *)	
	: BWindow(BRect(100,100,390,230), "Colors for Terminal", B_TITLED_WINDOW, B_NOT_RESIZABLE)
{
	
// Set up the main view	
	BView* icon = new BView(BRect(0,0,390,230), "Colors", B_FOLLOW_ALL, B_WILL_DRAW);



// Set up the colour of the panel and set it to being displayed
	AddChild(icon);
	icon->SetViewColor(216,216,216);
	
	Show();
	
	BMenuItem *menuItem;
	fWorkspaceMenu = new BPopUpMenu("pick one");
	fWorkspaceMenu->AddItem(menuItem = new BMenuItem("Text Foreground Color", new BMessage(MSG_COLOR_FIELD_CHANGED)));
	fWorkspaceMenu->AddItem(menuItem = new BMenuItem("Text Background Color", new BMessage(MSG_COLOR_FIELD_CHANGED)));
	fWorkspaceMenu->AddItem(menuItem = new BMenuItem("Cursor Foreground Color", new BMessage(MSG_COLOR_FIELD_CHANGED)));
	fWorkspaceMenu->AddItem(menuItem = new BMenuItem("Cursor Background Color", new BMessage(MSG_COLOR_FIELD_CHANGED)));
	fWorkspaceMenu->AddItem(menuItem = new BMenuItem("Selection Foreground Color", new BMessage(MSG_COLOR_FIELD_CHANGED)));
	fWorkspaceMenu->AddItem(menuItem = new BMenuItem("Selection Background Color", new BMessage(MSG_COLOR_FIELD_CHANGED)));
	//fWorkspaceMenu->AddSeparatorItem();
	BMenuField *workspaceMenuField = new BMenuField(BRect(10,10,160,30), "workspaceMenuField", NULL, fWorkspaceMenu, true);
	icon->AddChild(workspaceMenuField);
	workspaceMenuField->Menu()->ItemAt(0)->SetMarked (true);
	//rightbox->SetLabel(workspaceMenuField);

 // const char *color_tbl[]=
 // {
 //   PREF_TEXT_FORE_COLOR,
  //  PREF_TEXT_BACK_COLOR,
  //  PREF_CURSOR_FORE_COLOR,
  //  PREF_CURSOR_BACK_COLOR,
  //  PREF_SELECT_FORE_COLOR,
   // PREF_SELECT_BACK_COLOR,
   // NULL
  //};

 // fTermWindow = window;
 
  //
  // Color Control
  //
 // mColorField = SetupMenuField (IERECT(0, 60, 200, 20),
	//			"",
	//			MakeMenu (MSG_COLOR_FIELD_CHANGED,
	//			          color_tbl,
	//			          color_tbl[0]));

 // mColorCtl = SetupBColorControl(BPoint(20, 85),
	//			  B_CELLS_32x8,
	//			  6,
	//			  MSG_COLOR_CHANGED);
  
  BColorControl* controller = new BColorControl(BPoint(10, 45), B_CELLS_32x8, 6, "Terminal Color Controller", new BMessage(MSG_COLOR_CHANGED));
  
  icon->AddChild(controller);
  controller->SetValue(gTermPref->getRGB(PREF_TEXT_FORE_COLOR));



// Set up the BMessage handlers
}

void
colWindow::AttachedToWindow (void)
{
 // mHalfSize->SetTarget (this);
 // mFullSize->SetTarget (this);
 // mHalfFont->Menu()->SetTargetForItems(this);
  //mFullFont->Menu()->SetTargetForItems(this);  

  controller->SetTarget (this);
  workspaceMenuField->Menu()->SetTargetForItems(this);

}

void 
colWindow::MessageReceived(BMessage* msg)
{
	bool modified = false;
	switch(msg->what)
	{

	case MSG_COLOR_CHANGED:
    gTermPref->setRGB(workspaceMenuField->Menu()->FindMarked()->Label(),
		      controller->ValueAsColor());
	modified = true;
    break;

  case MSG_COLOR_FIELD_CHANGED:
    controller->SetValue
      (gTermPref->getRGB (workspaceMenuField->Menu()->FindMarked()->Label()));
    break;
	
	
	 default:
	 	BWindow::MessageReceived(msg);
	 	break;
	}
    
    if(modified){
   fTermWindow->PostMessage (msg);
  // (this->Window())->PostMessage(MSG_PREF_MODIFIED);
}
}
// Quit Button implementation	


void colWindow::Quit() 
{
	
	BWindow::Quit();
}


