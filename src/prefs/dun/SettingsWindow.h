/*

SettingsWindow Header 

Author: Misza (misza@ihug.com.au)

(C) 2002 OpenBeOS under MIT license

*/

#ifndef __SETTINGSWINDOW_H__
#define __SETTINGSWINDOW_H__

//Constants
const uint32 CHANGE_IP_BOX = 'Cipb';
const uint32 CHANGE_IP = 'CIP';
const uint32 CHANGE_S_DNS = 'SDns';
const uint32 CHANGE_P_DNS = 'PDns';
const uint32 BTN_SETTINGS_WINDOW_CANCEL = 'BSWC';
const uint32 BTN_SETTINGS_WINDOW_DONE = 'BSWD';

class SettingsView; 

class SettingsWindow : public BWindow
{
public:
   SettingsWindow(BRect frame);
   ~SettingsWindow();
   virtual void MessageReceived(BMessage *message);

private:
	void InitWindow();
	SettingsView* aSettingsview;

    BButton *btnSettingsWindowCancel;
    BButton *btnSettingsWindowDone;
   
    BPopUpMenu *YourServerTypeIsMenu;
    BMenuField *YourServerTypeIsMenuField;
   
    BTextControl* IPAddress;
    BTextControl* PrimaryDNS;
    BTextControl* SecondaryDNS;

	BStringView* ConnectSettings;
    BCheckBox *UseStaticIP;
	
	BBox* IPBox;
	BBox* DNSBox;
};

#endif
