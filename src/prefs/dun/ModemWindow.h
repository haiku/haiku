/*

ModemWindow Header

Author: Sikosis (beos@gravity24hr.com)

(C) 2002 OpenBeOS under MIT license

*/

#ifndef __MODEMWINDOW_H__
#define __MODEMWINDOW_H__

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
#include <Screen.h>
#include <Shape.h>
#include <stdio.h>
#include <TextControl.h>
#include <TextView.h>
#include <Window.h>
#include <View.h>

#include "DUNView.h"

// Constants
const uint32 BTN_MODEM_WINDOW_CANCEL = 'BMWC';
const uint32 BTN_MODEM_WINDOW_CUSTOM = 'BMWU';
const uint32 BTN_MODEM_WINDOW_DONE = 'BMWD';
const uint32 MENU = 'Menu';
const uint32 CHK_MAKE_CONNECTION = 'CKMC';
const uint32 CHK_SHOW_TERMINAL = 'CKST'; 
const uint32 CHK_LOG_ALL = 'CKLA';
const uint32 RB_TONE_DIALING = 'RBTn';
const uint32 RB_PULSE_DIALING = 'RBPd';


class ModemView; 

class ModemWindow : public BWindow
{
public:
   ModemWindow(BRect frame);
   ~ModemWindow();
   virtual void MessageReceived(BMessage *message);
private:
	void InitWindow(void);
	ModemView* aModemview;
   
    BButton *btnModemWindowCustom;
    BButton *btnModemWindowCancel;
    BButton *btnModemWindowDone;
   
    BMenu *YourModemIsMenu;
    BMenuField *YourModemIsMenuField;
    BMenu *ConnectViaMenu;
    BMenuField *ConnectViaMenuField;
    BMenu *SpeedMenu;
    BMenuField *SpeedMenuField;
   
    BTextView *tvRedial;
    BTextView *tvTimesBusySignal;
    BTextView *tvUse;
    BTextView *tvReadLogPath;
    BTextView *tvWriteLogPath;
   
    BCheckBox *chkMakeConnection;
    BCheckBox *chkShowTerminal;
    BCheckBox *chkLogAll;
   
    BRadioButton *rbTone;
    BRadioButton *rbPulse;
};

#endif
