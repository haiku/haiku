/*

ModemWindow - Modem Settings Window

Author: Sikosis (beos@gravity24hr.com)

(C) 2002 OpenBeOS under MIT license

*/

#include <stdlib.h>

#include "ModemWindow.h"


// ModemWindow -- constructor for ModemWindow Class
ModemWindow::ModemWindow(BRect frame) : BWindow (frame, "", B_MODAL_WINDOW, B_NOT_RESIZABLE , 0)
{
	InitWindow();
	CenterOnScreen();
	Show();
}
// ------------------------------------------------------------------------------- //

// ModemWindow::InitWindow -- Initialization Commands here
void ModemWindow::InitWindow(void)
{
    BRect r;
    r = Bounds();
   
    // Add the Drawing View
    aModemview = new ModemView(r);
    aModemview->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    AddChild(aModemview);
   
    // Buttons
    BRect btn1(10,r.bottom - 34,83,r.bottom - 16);
    BRect btn2(108,r.bottom - 34,184,r.bottom - 16);
    BRect btn3(196,r.bottom - 34,271,r.bottom - 16);
    btnModemWindowCustom = new BButton(btn1,"Custom","  Custom" B_UTF8_ELLIPSIS "  ", new BMessage(BTN_MODEM_WINDOW_CUSTOM), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
    btnModemWindowCancel = new BButton(btn2,"Cancel","  Cancel  ", new BMessage(BTN_MODEM_WINDOW_CANCEL), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
    btnModemWindowDone = new BButton(btn3,"Done","  Done  ", new BMessage(BTN_MODEM_WINDOW_DONE), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
   
    // YourModemIs MenuField
    BRect mfld1(15,20,350,30);
    BMenuItem *menu[100];
    YourModemIsMenu = new BMenu("<Pick One>                                 ");
    YourModemIsMenu->AddSeparatorItem();
    YourModemIsMenu->AddItem(menu[1] = new BMenuItem("New" B_UTF8_ELLIPSIS, new BMessage(MENU)));
    menu[1]->SetTarget(be_app);
    YourModemIsMenuField = new BMenuField(mfld1,"yourmodem_menufield","Your modem is:",YourModemIsMenu,B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
    YourModemIsMenuField->SetDivider(76);	
    YourModemIsMenuField->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
   
    // ConnectVia MenuField
    BRect mfld2(28,45,350,55);
    BMenuItem *menu2[100];
    ConnectViaMenu = new BMenu("serial 1   ");
    
    //ConnectViaMenu->AddSeparatorItem();
    ConnectViaMenu->AddItem(menu2[1] = new BMenuItem("serial 2   ", new BMessage(MENU)));
    menu2[1]->SetTarget(be_app);
    ConnectViaMenuField = new BMenuField(mfld2,"connectvia_menufield","Connect via:",ConnectViaMenu,B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
    ConnectViaMenuField->SetDivider(63);	
    ConnectViaMenuField->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    
    // Speed MenuField
    BRect mfld3(54,70,350,80);
    BMenuItem *menu3[100];
    SpeedMenu = new BMenu("9600    ");
    SpeedMenu->AddItem(menu3[1] = new BMenuItem("19200   ", new BMessage(MENU)));
    menu3[1]->SetTarget(be_app);
    SpeedMenuField = new BMenuField(mfld3,"speed_menufield","Speed:",SpeedMenu,B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
    SpeedMenuField->SetDivider(37);	
    SpeedMenuField->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
 
    // TextView - Redial
    BRect RedialLocation (15,112,350,132);
    tvRedial = new BTextView(r, "Redial", RedialLocation, B_FOLLOW_ALL, B_WILL_DRAW);
    tvRedial->SetText("Redial");
    tvRedial->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    tvRedial->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    tvRedial->MakeSelectable(false);
    tvRedial->MakeEditable(false);
	
    // TextView - Use:
    BRect UseLocation (15,142,40,154);
    tvUse = new BTextView(r, "Use", UseLocation, B_FOLLOW_ALL, B_WILL_DRAW);
    tvUse->SetText("Use:");
    tvUse->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    tvUse->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    tvUse->MakeSelectable(false);
    tvUse->MakeEditable(false);
    
    // RadioButton - Use Tone or Pulse
	BRect RBToneLocation(42,140,350,152);
	rbTone = new BRadioButton(RBToneLocation,"Tone","Tone dialing", new BMessage(RB_TONE_DIALING), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	BRect RBPulseLocation(125,140,350,152);
	rbPulse = new BRadioButton(RBPulseLocation,"Pulse","Pulse dialing", new BMessage(RB_PULSE_DIALING), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
    
    // TextView - TimesBusySignal
    BRect TimesBusySignalLocation (82,112,400,132);
    tvTimesBusySignal = new BTextView(r, "TimesBusySignal", TimesBusySignalLocation, B_FOLLOW_ALL, B_WILL_DRAW);
    tvTimesBusySignal->SetText("time(s) on busy signal");
    tvTimesBusySignal->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    tvTimesBusySignal->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    tvTimesBusySignal->MakeSelectable(false);
    tvTimesBusySignal->MakeEditable(false);
    
	// TextView - ReadLogPath
	BRect ReadLogPathLocation (32,238,350,258);
	tvReadLogPath = new BTextView(r, "ReadLogPath", ReadLogPathLocation, B_FOLLOW_ALL, B_WILL_DRAW);
	tvReadLogPath->SetText("Read log file path: /boot/var/log/ppp-read.log");
 	tvReadLogPath->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    tvReadLogPath->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
 	tvReadLogPath->MakeSelectable(false);
	tvReadLogPath->MakeEditable(false);
	
	// TextView - WriteLogPath
	BRect WriteLogPathLocation (32,256,350,276);
	tvWriteLogPath = new BTextView(r, "WriteLogPath", WriteLogPathLocation, B_FOLLOW_ALL, B_WILL_DRAW);
	tvWriteLogPath->SetText("Write log file path: /boot/var/log/ppp-write.log");
 	tvWriteLogPath->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    tvWriteLogPath->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
 	
 	tvWriteLogPath->MakeSelectable(false);
	tvWriteLogPath->MakeEditable(false);

	// CheckBox - chkMakeConnection
	BRect chkMakeConnectionLocation (15,159,300,179);
	chkMakeConnection = new BCheckBox(chkMakeConnectionLocation, "chkMakeConnection", "Make connection when necessary", new BMessage(CHK_MAKE_CONNECTION), B_FOLLOW_LEFT | B_FOLLOW_TOP , B_WILL_DRAW | B_NAVIGABLE); 
	chkMakeConnection->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	// CheckBox - chkShowTerminal
	BRect chkShowTerminalLocation (15,196,300,216);
	chkShowTerminal = new BCheckBox(chkShowTerminalLocation, "chkShowTerminal", "Show terminal when connecting", new BMessage(CHK_SHOW_TERMINAL), B_FOLLOW_LEFT | B_FOLLOW_TOP , B_WILL_DRAW | B_NAVIGABLE); 
	chkShowTerminal->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    
    // CheckBox - chkLogAll
	BRect chkLogAllLocation (15,215,300,235);
	chkLogAll = new BCheckBox(chkLogAllLocation, "chkLogAll", "Log all bytes sent/received", new BMessage(CHK_LOG_ALL), B_FOLLOW_LEFT | B_FOLLOW_TOP , B_WILL_DRAW | B_NAVIGABLE); 
	chkLogAll->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
       
    // Add Objects to View 
    aModemview->AddChild(YourModemIsMenuField);
    aModemview->AddChild(ConnectViaMenuField);
    aModemview->AddChild(SpeedMenuField);
    aModemview->AddChild(btnModemWindowCancel);   
    aModemview->AddChild(btnModemWindowCustom);   
    aModemview->AddChild(btnModemWindowDone);  
    aModemview->AddChild(chkMakeConnection);
    aModemview->AddChild(chkShowTerminal);
    aModemview->AddChild(chkLogAll);
    aModemview->AddChild(rbPulse);
    aModemview->AddChild(rbTone);
    aModemview->AddChild(tvUse);
    aModemview->AddChild(tvRedial); 
    aModemview->AddChild(tvTimesBusySignal);
    aModemview->AddChild(tvReadLogPath);
    aModemview->AddChild(tvWriteLogPath);
       
    btnModemWindowDone->MakeDefault(true);
   
}
// ------------------------------------------------------------------------------- //


// ModemWindow::~ModemWindow -- destructor
ModemWindow::~ModemWindow()
{
	exit(0);
}
// ------------------------------------------------------------------------------- //


// ModemWindow::MessageReceived -- receives messages
void ModemWindow::MessageReceived (BMessage *message)
{
	switch(message->what)
	{
   		case BTN_MODEM_WINDOW_DONE:
   	    	//Quit(); // debugging purposes
   	   		Hide();
   	   		break;	
		case BTN_MODEM_WINDOW_CANCEL:
			Hide(); // must find a better way to close this window and not the entire application
					// any suggestions would be greatly appreciated ;)
   		   	break;
    	default:
	    	BWindow::MessageReceived(message);
    	    break;
    }
}
// end
