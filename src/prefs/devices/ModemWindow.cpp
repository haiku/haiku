/*

ModemWindow

Author: Sikosis

(C)2003 OBOS - Released under the MIT License

*/

// Includes ------------------------------------------------------------------------------------------ //
#include <Application.h>
#include <Box.h>
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

/*BBox(BRect frame, const char *name = NULL, 
  
    uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP, 
      uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS |  
            B_NAVIGABLE_JUMP, 
      border_style border = B_FANCY_BORDER)*/


// ModemWindow::InitWindow -- Initialization Commands here
void ModemWindow::InitWindow(void)
{
	BRect r;
	r = Bounds();
	
	boxInternalModem = new BBox(BRect (r.left+10,r.top+15,r.right-10,r.bottom-50),
						 " Internal Modem ", B_FOLLOW_LEFT | B_FOLLOW_TOP,
						 B_WILL_DRAW, B_FANCY_BORDER);
	
	BRect CancelButtonRect(24,r.bottom-37,100,r.bottom-13);
	btnCancel = new BButton(CancelButtonRect,"btnCancel","Cancel",
				 	new BMessage(BTN_CANCEL), B_FOLLOW_LEFT | B_FOLLOW_TOP,
				 	B_WILL_DRAW | B_NAVIGABLE);
	
	BRect AddButtonRect(115,r.bottom-37,r.right - 10,r.bottom-13);
	btnAdd = new BButton(AddButtonRect,"btnAdd","Add",
				 	new BMessage(BTN_ADD), B_FOLLOW_LEFT | B_FOLLOW_TOP,
				 	B_WILL_DRAW | B_NAVIGABLE);
	btnAdd->MakeDefault(true);			 	
		
	// Create the Views
	AddChild(ptrModemView = new ModemView(r));
	ptrModemView->AddChild(boxInternalModem);
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





