/*

DetailsView - View for Details

Author: Misza (misza@ihug.com.au) 

(C) 2002 OpenBeOS under MIT license

*/


#include "DetailsView.h"

using std::cout;
using std::endl;

DetailsView::DetailsView() : 
BView(BRect(40,31,290,157),"detailsview",B_FOLLOW_NONE,B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BRect btnr = Bounds();
	BRect texties(0,0,239,20);
	phoneno = new BTextControl(texties,"phoneno","Phone number:",NULL,
	new BMessage(CHANGE_PHONENO));
	phoneno->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	phoneno->SetDivider(80);
	AddChild(phoneno);
	
	texties.OffsetBy(0,24);
	username = new BTextControl(texties,"username","User name:",NULL,
	new BMessage(CHANGE_USERNAME));
	username->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	username->SetDivider(80);
	
	AddChild(username);
	
	texties.OffsetBy(0,24);
	passwd = new BTextControl(texties,"passwd","Password:",NULL,
	new BMessage(CHANGE_PASSWORD));
	passwd->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	passwd->SetDivider(80);
	//we don't want any people seeing our "3y3lurveBe05" password ;)
	passwd->TextView()->HideTyping(true);
	AddChild(passwd);
	
	texties.OffsetBy(0,24);
	texties.left += 82;
	savepasswd = new BCheckBox(texties,"savepasswd","Save Password", new BMessage(SAVEPASSWORD),B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	savepasswd->SetLabel("Save Password");
	AddChild(savepasswd);
	
	btnr.right -= 8;
	btnr.bottom -= 8;
	btnr.top = btnr.bottom - 24;
	btnr.left = btnr.right - 70;
   
	settingsbtn = new BButton(btnr,"Settings","Settings" B_UTF8_ELLIPSIS", new BMessage(BTN_SETTINGS), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	AddChild(settingsbtn);
}
void DetailsView::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
	case CHANGE_PHONENO:
	case CHANGE_USERNAME:
	case CHANGE_PASSWORD:
	case SAVEPASSWORD:
	
	cout << "\n\nPhone number: ";
	cout << phoneno->Text();
	cout << "\nUser name: ";
	cout << username->Text();
	cout << "\nPassword: ";
	cout << passwd->Text();
	cout << "\nSave Password? ";
	if(savepasswd->Value() == B_CONTROL_ON)
	
		cout << "YES\n" << endl;
	else
		cout << "NO\n" << endl;
	break;
	
		default:
        BView::MessageReceived(msg);
         break;
	}
}
void DetailsView::AttachedToWindow()
{
	phoneno->SetTarget(this);
	username->SetTarget(this);
	passwd->SetTarget(this);
	savepasswd->SetTarget(this);
}
