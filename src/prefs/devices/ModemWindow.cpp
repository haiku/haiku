/*

ModemWindow

Author: Sikosis

(C)2003 OBOS - Released under the MIT License

*/

// Includes ------------------------------------------------------------------------------------------ //
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <stdio.h>
#include <String.h>
#include <TextView.h>
#include <Window.h>
#include <View.h>

#include "DevicesWindows.h"

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
ModemWindow::ModemWindow(BRect frame, BMessenger messenger) 
	: BWindow (frame, "ModemWindow", B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE),
	fMessenger(messenger)
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
	
	BBox *background = new BBox(r, "background", B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW, B_PLAIN_BORDER);
	AddChild(background);
	
	BBox *boxInternalModem = new BBox(BRect (r.left+9,r.top+9,r.right-11,r.bottom-44),
						 "boxInternalModem", B_FOLLOW_LEFT | B_FOLLOW_TOP,
						 B_WILL_DRAW, B_FANCY_BORDER);
	boxInternalModem->SetLabel("Internal Modem");
	boxInternalModem->SetFont(be_plain_font);
	
	rgb_color black = {0,0,0};
	BRect rect = boxInternalModem->Bounds();
	rect.left += 9;
	rect.top += 58;
	rect.right -= 11;
	rect.bottom -= 20;
	BRect textRect(rect);
	textRect.OffsetTo(B_ORIGIN);
	textRect.InsetBy(1,1);
	BTextView *textView = new BTextView(rect, "text", textRect, be_plain_font, &black, B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW);
	textView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	textView->SetText("This is intended for configuring\n"
		"jumpered modem only. Plug and\n"
		"Play modems are automatically\n"
		"detected by the system.");
	boxInternalModem->AddChild(textView);
	
	BRect CancelButtonRect(29,r.bottom-34,104,r.bottom-13);
	BButton *btnCancel = new BButton(CancelButtonRect,"btnCancel","Cancel",
				 	new BMessage(BTN_CANCEL), B_FOLLOW_LEFT | B_FOLLOW_TOP,
				 	B_WILL_DRAW | B_NAVIGABLE);
	
	BRect AddButtonRect(115,r.bottom-34,r.right - 10,r.bottom-13);
	BButton *btnAdd = new BButton(AddButtonRect,"btnAdd","Add",
				 	new BMessage(BTN_ADD), B_FOLLOW_LEFT | B_FOLLOW_TOP,
				 	B_WILL_DRAW | B_NAVIGABLE);
	btnAdd->MakeDefault(true);
	
	rect = boxInternalModem->Bounds(); 	
	rect.top += 28;
	rect.left += 38;
	rect.right = rect.left + 105;
	rect.bottom = rect.top + 5;
	BPopUpMenu *serialMenu = new BPopUpMenu("pick one");
	BMenuItem *item;
	serialMenu->AddItem(item = new BMenuItem("serial3  ", NULL));
	item->SetMarked(true);
	serialMenu->AddItem(item = new BMenuItem("serial4  ", NULL));
	
	
	BMenuField *serialMenuField = new BMenuField(rect, "serialMenuField",
		"Port:", serialMenu);
	serialMenuField->SetDivider(40.0);
	serialMenuField->SetAlignment(B_ALIGN_RIGHT);
	boxInternalModem->AddChild(serialMenuField);
		
	// Create the Views
	background->AddChild(boxInternalModem);
	background->AddChild(btnCancel);
	background->AddChild(btnAdd);
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
		case BTN_ADD:
			{
				fMessenger.SendMessage(MODEM_ADDED);
				Quit();
			}
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
// -------------------------------------------------------------------------------------------------- //





