/*

ModemWindow

Author: Sikosis

(C)2003 OBOS - Released under the MIT License

*/

// Includes ------------------------------------------------------------------------------------------ //
#include <Application.h>
#include <Button.h>
#include <Path.h>
#include <Screen.h>
#include <stdio.h>
#include <String.h>
#include <Window.h>
#include <View.h>

#include "Devices.h"
#include "DevicesWindows.h"
#include "DevicesViews.h"
// -------------------------------------------------------------------------------------------------- //
const uint32 BTN_ADD = 'badd';
const uint32 BTN_CANCEL = 'bcnl';

// CenterWindowOnScreen -- Centers the BWindow to the Current Screen
static void CenterWindowOnScreen(BWindow* w)
{
	BRect screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint pt;
	pt.x = screenFrame.Width()/2 - w->Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - w->Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		w->MoveTo(pt);
}
// -------------------------------------------------------------------------------------------------- //


// ModemWindow - Constructor
ModemWindow::ModemWindow(BRect frame) : BWindow (frame, "ModemWindow", B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, 0)
{
	InitWindow();
	CenterWindowOnScreen(this);
	Show();
}
// -------------------------------------------------------------------------------------------------- //


// ModemWindow - Destructor
ModemWindow::~ModemWindow()
{
	//exit(0);
}
// -------------------------------------------------------------------------------------------------- //


// ModemWindow::InitWindow -- Initialization Commands here
void ModemWindow::InitWindow(void)
{
	BRect r;
	r = Bounds();
	
	BRect CancelButtonRect(27,r.bottom-35,103,r.bottom-15);
	btnCancel = new BButton(CancelButtonRect,"btnCancel","Cancel",
				 	new BMessage(BTN_CANCEL), B_FOLLOW_LEFT | B_FOLLOW_TOP,
				 	B_WILL_DRAW | B_NAVIGABLE);
	
	BRect AddButtonRect(110,r.bottom-35,186,r.bottom-15);
	btnAdd = new BButton(AddButtonRect,"btnAdd","Add",
				 	new BMessage(BTN_ADD), B_FOLLOW_LEFT | B_FOLLOW_TOP,
				 	B_WILL_DRAW | B_NAVIGABLE);
		
	// Create the Views
	AddChild(ptrModemView = new ModemView(r));
	ptrModemView->AddChild(btnCancel);
	ptrModemView->AddChild(btnAdd);
}
// -------------------------------------------------------------------------------------------------- //


// ModemWindow::MessageReceived -- receives messages
void ModemWindow::MessageReceived (BMessage *message)
{
	switch(message->what)
	{
		case BTN_CANCEL:
			{
				Quit();
			}
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
// -------------------------------------------------------------------------------------------------- //





