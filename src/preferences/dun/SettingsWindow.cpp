/*

SettingsWindow - Window for Settings

Author: Misza (misza@ihug.com.au) 

(C) 2002 OpenBeOS under MIT license

*/

#include <Application.h>
#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <Font.h>
#include <ListItem.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <OutlineListView.h>
#include <Point.h>
#include <RadioButton.h>
#include <Shape.h>
#include <stdio.h>
#include <TextControl.h>
#include <TextView.h>
#include <Window.h>
#include <View.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include "DUN.h"
//#include "DUNWindow.h"
//#include "ModemWindow.h"
#include "DUNView.h"
#include "SettingsWindow.h"


// SettingsWindow -- constructor for SettingsWindow Class
SettingsWindow::SettingsWindow(BRect frame) : BWindow (frame, "", B_MODAL_WINDOW, B_NOT_RESIZABLE , 0)
{
	InitWindow();
	Show();
}
// ------------------------------------------------------------------------------- //


// Settingsview::InitWindow -- Initialization Commands here
void SettingsWindow::InitWindow()
{
    BRect r;
    r = Bounds();
   
    // Add the Drawing View
    aSettingsview = new SettingsView(r);
    aSettingsview->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    AddChild(aSettingsview);
	
	//aSettingsview->SetFont(be_bold_font);
	
	//THIS should be changed later, I know no other way of setting this :PPP
	ConnectSettings = new BStringView(BRect(13,5,250,25),"ConnectSettings","Connection settings for: ");
	//aSettingsview->MovePenTo(30,40);
	//aSettingsview->DrawString("Connection settings for: ");
	ConnectSettings->SetFont(be_bold_font);
	aSettingsview->AddChild(ConnectSettings);
	
	aSettingsview->SetFont(be_plain_font);
	//aSettingsview->DrawString("some_connection");
	BRect ipbox_rect = aSettingsview->Bounds();
	ipbox_rect.left = 13;
	ipbox_rect.right -= 13;
	ipbox_rect.top = 32;
	ipbox_rect.bottom = 81;
	//ipbox_rect.InsetBy(20,20);
	IPBox = new BBox(ipbox_rect,"IPBox",B_FOLLOW_ALL | B_WILL_DRAW | B_NAVIGABLE_JUMP, B_FANCY_BORDER);
   
    //IPBox->SetLabel("<BCheckBox>Use static IP address");
    	BView* iplabel = new BView(BRect(10,10,130,30),"iplabel",B_FOLLOW_ALL_SIDES,B_WILL_DRAW|B_NAVIGABLE);
     //iplabel->SetViewColor(255,0,0);
     iplabel->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    
    //BRect stat_ip_rect = iplabel->Bounds();
    //stat
    UseStaticIP = new BCheckBox(iplabel->Bounds(),"UseStaticIP","Use static IP address", new BMessage(CHANGE_IP_BOX),B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	UseStaticIP->SetLabel("Use static IP address");
	//UseStaticIP->SetEnabled(false);//by default it is off
	iplabel->AddChild(UseStaticIP);
    
    // IPBox->SetLabel("<BCheckBox>Use static IP address");
    IPBox->SetLabel(iplabel);
    
//	ipbox_rect.left = 23;
//	ipbox_rect.bottom += 
	BRect ipaddr_text = IPBox->Bounds();
	ipaddr_text.right -= 10;
	ipaddr_text.left += 50;
	ipaddr_text.top += 20;
	IPAddress = new BTextControl(ipaddr_text,"IPAddress","IP address:",NULL,
	new BMessage(CHANGE_IP));
	IPAddress->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	IPAddress->SetDivider(80);
	IPAddress->SetEnabled(false); //by default it is off
	
	IPBox->AddChild(IPAddress);
	
	aSettingsview->AddChild(IPBox);

	ipbox_rect.top += 60;
	ipbox_rect.bottom += 75;
	
	DNSBox = new BBox(ipbox_rect,"IPBox",B_FOLLOW_ALL | B_WILL_DRAW | B_NAVIGABLE_JUMP, B_FANCY_BORDER);
	
	ipaddr_text.top -= 10;
	
	PrimaryDNS = new BTextControl(ipaddr_text,"PrimaryDNS","Primary DNS:",NULL,
	new BMessage(CHANGE_P_DNS));
	PrimaryDNS->SetDivider(80);
	PrimaryDNS->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	DNSBox->AddChild(PrimaryDNS);
	ipaddr_text.top += 25;
	SecondaryDNS = new BTextControl(ipaddr_text,"SecondaryDNS","Secondary DNS:",NULL,
	new BMessage(CHANGE_S_DNS));
	SecondaryDNS->SetDivider(80);
	SecondaryDNS->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	DNSBox->AddChild(SecondaryDNS);
	
    aSettingsview->AddChild(DNSBox);
	BRect cute_buttons(205,200,280,220);
	btnSettingsWindowDone = new BButton(cute_buttons,"btnSettingsWindowDone","  Done  ", new BMessage(BTN_SETTINGS_WINDOW_DONE), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
    cute_buttons.OffsetBy(-85,0);
	btnSettingsWindowCancel = new BButton(cute_buttons,"btnSettingsWindowCancel","  Cancel  ", new BMessage(BTN_SETTINGS_WINDOW_CANCEL), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);

    btnSettingsWindowDone->MakeDefault(true);
    aSettingsview->AddChild(btnSettingsWindowDone);
	aSettingsview->AddChild(btnSettingsWindowCancel);

	YourServerTypeIsMenu = new BPopUpMenu("ServerType");
	
	//These should be added after looking in /boot/beos/etc/servers.ppp
	//parse that files contents, put the entries in a list, whatever
	//loop through the list and add them
	BMenuItem* item1 = new BMenuItem("Standard PPP",NULL); //n.b bmessage left out, add later
	YourServerTypeIsMenu->AddItem(item1);
	
	BMenuItem* item2 = new BMenuItem("Etc" B_UTF8_ELLIPSIS,NULL);
	YourServerTypeIsMenu->AddItem(item2);
	
	
	YourServerTypeIsMenuField = new BMenuField(BRect(13,170,203,190),
	"ServerType","Server Type:",YourServerTypeIsMenu);
	item1->SetMarked(true);
	YourServerTypeIsMenuField->SetDivider(70);
	aSettingsview->AddChild(YourServerTypeIsMenuField);
	
}
// ------------------------------------------------------------------------------- //


// SettingsWindow::~aSettingsview -- destructor
SettingsWindow::~SettingsWindow()
{
	exit(0);
}
// ------------------------------------------------------------------------------- //


// SettingsWindow::MessageReceived -- receives messages
void SettingsWindow::MessageReceived (BMessage *message)
{
	switch(message->what)
	{
   		case CHANGE_IP_BOX:
   		{
   			if(UseStaticIP->Value() == B_CONTROL_ON){	
   			/*do something*/
   			
   			//cout << "checkbox is on" << endl;
   			IPAddress->SetEnabled(true);
   			}else
   			{
   			/*do something else*/
   			//cout << "checkbox is off" << endl;
   			IPAddress->SetEnabled(false);
   			
   			}
   			
   			}break;
   		case CHANGE_IP:
   		{
   			//cout << IPAddress->Text() << endl;
   			printf("%s\n",IPAddress->Text());
   		}break;
   		
   		case CHANGE_P_DNS:
   		{
   		
   	    //cout << PrimaryDNS->Text() << endl;
		printf("%s\n",PrimaryDNS->Text());
   		
   		}break;
   		case CHANGE_S_DNS:
   		{
   		
   		//cout << SecondaryDNS->Text() << endl;
		printf("%s\n",SecondaryDNS->Text());
   		}break;
   		
   		case BTN_SETTINGS_WINDOW_CANCEL:
   		{
   		Hide(); // must find a better way to close this window and not the entire application
					// any suggestions would be greatly appreciated ;)
   		}break;
		case BTN_SETTINGS_WINDOW_DONE:
   		{
   		/*SAVE SETTINGS*/
   		
   		Hide();
   		//Quit();
   		}break;
   		/*case BTN_MODEM_WINDOW_DONE:
   	    	Quit(); // debugging purposes
   	   		break;	
		case BTN_MODEM_WINDOW_CANCEL:
			Hide(); // must find a better way to close this window and not the entire application
					// any suggestions would be greatly appreciated ;)
   		   	break;*/
    	default:
	    	BWindow::MessageReceived(message);
    	    break;
    }
}
// end
