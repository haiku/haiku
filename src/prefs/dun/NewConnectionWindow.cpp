/*

NewConnectionWindow - DialUp Networking

Author: Sikosis (beos@gravity24hr.com)

(C) 2002 OpenBeOS under MIT license

*/

// Includes -------------------------------------------------------------------------------------------------- //
#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <Directory.h>
#include <Path.h>
#include <Screen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <TextControl.h>
#include <Window.h>
#include <View.h>

#include "NewConnectionWindow.h"
#include "DUNView.h"

// Constants ------------------------------------------------------------------------------------------------- //
const uint32 BTN_OKAY = 'BOky';
const uint32 BTN_CANCEL = 'BCnl';
const uint32 TXT_NEW_CONNECTION = 'TxCx';
// ---------------------------------------------------------------------------------------------------------- //


// CenterWindowOnScreen -- Centers the BWindow to the Current Screen
static void CenterWindowOnScreen(BWindow* w)
{
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - w->Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - w->Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		w->MoveTo(pt);
}
// ---------------------------------------------------------------------------------------------------------- //


// NewConnectionWindow - Constructor
NewConnectionWindow::NewConnectionWindow(BRect frame) : BWindow (frame, "NewConnectionWindow", B_MODAL_WINDOW , B_NOT_RESIZABLE , 0)
{
	InitWindow();
	CenterWindowOnScreen(this);
	Show();
}


// NewConnectionWindow - Destructor
NewConnectionWindow::~NewConnectionWindow()
{
	//exit(0);
}


// NewConnectionWindow::InitWindow
void NewConnectionWindow::InitWindow(void)
{	
	BRect r;
	r = Bounds(); // the whole view
	
	int LeftMargin = 6;
	int OkayButtonSize = 60;
	int CancelButtonSize = 60;
	
	float CancelLeftMargin = (r.right / 2) - ((OkayButtonSize + 20 + CancelButtonSize) / 2);
	float OkayLeftMargin = CancelLeftMargin + CancelButtonSize + 20;;
	
	int NewConnectionTop = 20;
	
	txtNewConnection = new BTextControl(BRect(LeftMargin,NewConnectionTop,r.right-LeftMargin+2,NewConnectionTop+10), "txtNewConnection","New Connection:","New Connection",new BMessage(TXT_NEW_CONNECTION), B_FOLLOW_LEFT | B_FOLLOW_TOP , B_WILL_DRAW | B_NAVIGABLE);
	txtNewConnection->SetDivider(65);
	   	
	btnCancel = new BButton(BRect (CancelLeftMargin,r.bottom-35,CancelLeftMargin+CancelButtonSize,r.bottom-15),"Cancel","Cancel", new BMessage(BTN_CANCEL), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);		
  	btnOkay = new BButton(BRect (OkayLeftMargin,r.bottom-35,OkayLeftMargin+OkayButtonSize,r.bottom-15),"Okay","Okay", new BMessage(BTN_OKAY), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
  	btnOkay->MakeDefault(true);
		
	//AddChild(ptrNewConnectionWindowView = new NewConnectionWindowView(r));
	AddChild(txtNewConnection);
	AddChild(btnCancel);
    AddChild(btnOkay);
    txtNewConnection->MakeFocus(true);
}
// ---------------------------------------------------------------------------------------------------------- //

// NewConnectionWindow::MessageReceived -- receives messages
void NewConnectionWindow::MessageReceived (BMessage *message)
{
	switch(message->what)
	{
		case BTN_OKAY:
			char tmp[256];
			sprintf(tmp,"%s",txtNewConnection->Text());
			(new BAlert("",tmp,"tmp"))->Go();
			Hide(); // change later 
			break;
		case BTN_CANCEL:
			Hide(); // change later 
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
// ---------------------------------------------------------------------------------------------------------- //


