/*

DUNWindow by Sikosis (beos@gravity24hr.com)

(C) 2002 OpenBeOS under MIT license

*/

#include "app/Application.h"
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
#include <stdio.h>
#include <TextControl.h>
#include <TextView.h>
#include <Window.h>
#include <View.h>

#include "DUN.h"
#include "DUNWindow.h"
#include "ModemWindow.h"
#include "DUNView.h"

// Constants
const uint32 BTN_MODEM = 'Modm';
const uint32 BTN_DISCONNECT = 'Disc';
const uint32 BTN_CONNECT = 'Conn';
const uint32 CHK_CALLWAITING = 'Ckcw';
const uint32 CHK_DIALOUTPREFIX = 'Ckdp';

const uint32 MENU_CLICKTOADD = 'MC2A';
const uint32 MENU_CON_NEW = 'MCNu';
const uint32 MENU_CON_DELETE_CURRENT = 'MCDl';

const uint32 MENU_LOC_HOME = 'MLHm';
const uint32 MENU_LOC_WORK = 'MLWk';
const uint32 MENU_LOC_NEW = 'MLNu';
const uint32 MENU_LOC_DELETE_CURRENT = 'MLDl';

float ModemFormTopDefault = 150;
float ModemFormLeftDefault = 150;
float ModemFormWidthDefault = 281;
float ModemFormHeightDefault = 324;
BRect windowRectModem(ModemFormTopDefault,ModemFormLeftDefault,ModemFormLeftDefault+ModemFormWidthDefault,ModemFormTopDefault+ModemFormHeightDefault);

// DUNWindow -- constructor for DUNWindow Class
DUNWindow::DUNWindow(BRect frame) : BWindow (frame, "OBOS Dial-up Networking", B_TITLED_WINDOW, B_NOT_RESIZABLE , 0) {
   InitWindow();
   Show();
   
}
// ------------------------------------------------------------------------------- //

// DUNWindow::InitWindow -- Initialization Commands here
void DUNWindow::InitWindow(void) {
   BRect r;
   r = Bounds();
   
   // Buttons
   float ButtonTop = 217;
   float ButtonWidth = 24;
   BRect btn1(10,ButtonTop,83,ButtonTop+ButtonWidth);
   modembutton = new BButton(btn1,"Modem","Modem...", new BMessage(BTN_MODEM), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
   BRect btn2(143,ButtonTop,218,ButtonTop+ButtonWidth);
   disconnectbutton = new BButton(btn2,"Disconnect","Disconnect", new BMessage(BTN_DISCONNECT), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
   BRect btn3(230,ButtonTop,302,ButtonTop+ButtonWidth);
   connectbutton = new BButton(btn3,"Connect","Connect", new BMessage(BTN_CONNECT), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
   
   // Check Boxes -- Only when form in state 2
   //BRect chk1(68,110,220,110);
   //disablecallwaiting = new BCheckBox(chk1,"Disable call waiting","Disable call waiting:", new BMessage(CHK_CALLWAITING), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
   //BRect chk2(68,137,220,110);
   //dialoutprefix = new BCheckBox(chk2,"Dial out prefix","Dial out prefix:", new BMessage(CHK_DIALOUTPREFIX), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
   
   // Connection MenuField
   BRect mfld1(15,8,180,28);
   BMenuItem *menuconnew;
   BMenuItem *menucondelete;

   conmenufield = new BMenu("Click to add");
   conmenufield->AddSeparatorItem();
   conmenufield->AddItem(menuconnew = new BMenuItem("New...", new BMessage(MENU_CON_NEW)));
   menuconnew->SetTarget(be_app);
   conmenufield->AddItem(menucondelete = new BMenuItem("Delete Current", new BMessage(MENU_CON_DELETE_CURRENT)));
   menucondelete->SetEnabled(false);
   menucondelete->SetTarget(be_app);
   connectionmenufield = new BMenuField(mfld1,"connection_menu","Connect to:",conmenufield,B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
   connectionmenufield->SetDivider(77);	
   connectionmenufield->SetFont(be_bold_font);
   connectionmenufield->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
   
   // Location MenuField
   BRect mfld2(mfld1.left,75,250,125);
   BMenuItem *menulochome;
   BMenuItem *menulocwork;
   BMenuItem *menulocnew;
   BMenuItem *menulocdelete;
   
   locmenufield = new BMenu("Home            ");
   locmenufield->AddItem(menulochome = new BMenuItem("Home", new BMessage(MENU_LOC_HOME)));
   menulochome->SetTarget(be_app);
   menulochome->SetMarked(true);

   locmenufield->AddItem(menulocwork = new BMenuItem("Work", new BMessage(MENU_LOC_WORK)));
   menulocwork->SetTarget(be_app);
   locmenufield->AddSeparatorItem();
   
   locmenufield->AddItem(menulocnew = new BMenuItem("New...", new BMessage(MENU_LOC_NEW)));
   menulocnew->SetTarget(be_app);
   
   locmenufield->AddItem(menulocdelete = new BMenuItem("Delete Current", new BMessage(MENU_LOC_DELETE_CURRENT)));
   menulocdelete->SetTarget(be_app);
   
   locationmenufield = new BMenuField(mfld2,"location_menu","From location:",locmenufield,B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
   locationmenufield->SetDivider(88);	
   locationmenufield->SetFont(be_bold_font);
   locationmenufield->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
   
   // Displays - <Create a connection profile to continue.> on the main window
   BRect CPLocation(40,43,300,10);
   tvConnectionProfile = new BTextView(r, "Connection Profile", CPLocation, B_FOLLOW_ALL, B_WILL_DRAW);
   tvConnectionProfile->SetText("<Create a connection profile to continue.>");
   tvConnectionProfile->MakeSelectable(false);
   tvConnectionProfile->MakeEditable(false);
   
   // Displays - Call waiting may be enabled.
   BRect CWLocation(40,113,300,10);
   tvCallWaiting = new BTextView(r, "Call waiting", CWLocation, B_FOLLOW_ALL, B_WILL_DRAW);
   tvCallWaiting->SetText("Call waiting may be enabled.");
   tvCallWaiting->MakeSelectable(false);
   tvCallWaiting->MakeEditable(false);
   
   // Displays - No Connection
   BRect NCLocation(21,168,300,10);
   tvConnection = new BTextView(r, "No Connection", NCLocation, B_FOLLOW_ALL, B_WILL_DRAW);
   tvConnection->SetText("No Connection");
   tvConnection->MakeSelectable(false);
   tvConnection->MakeEditable(false);
   
   // Display - Time Online ie. 00:00:00
   BRect TOLocation(249,168,300,10);
   tvTimeOnline = new BTextView(r, "Time Online", TOLocation, B_FOLLOW_ALL, B_WILL_DRAW);
   tvTimeOnline->SetText("00:00:00");
   tvTimeOnline->MakeSelectable(false);
   tvTimeOnline->MakeEditable(false);
   
   // Display - TextView of Local IP address
   BRect TextLIPLocation(21,186,300,10);
   tvLIP = new BTextView(r, "Local IP address", TextLIPLocation, B_FOLLOW_ALL, B_WILL_DRAW);
   tvLIP->SetText("Local IP address:");
   tvLIP->MakeSelectable(false);
   tvLIP->MakeEditable(false);
   
   // Display - Local IP address
   BRect LIPLocation (264,186,300,10);
   tvLocalIPAddress = new BTextView(r, "None", LIPLocation, B_FOLLOW_ALL, B_WILL_DRAW);
   tvLocalIPAddress->SetText("None");
   tvLocalIPAddress->MakeSelectable(false);
   tvLocalIPAddress->MakeEditable(false);
        
   // Outline List View - Fake ones really as we're only using them as an indicator.
   BListItem *conitem;
   BListItem *consubitem;
   BRect lst1(20,44,100,80);
   BListItem *locitem;
   BListItem *locsubitem;
   BRect lst2(20,114,100,170);
     
   connectionlistitem = new BOutlineListView(lst1, "connection_list", B_MULTIPLE_SELECTION_LIST);
   connectionlistitem->AddItem(conitem = new BStringItem(""));
   connectionlistitem->AddUnder(consubitem = new BStringItem("\0"), conitem);
   connectionlistitem->Collapse(conitem);
  
  /* BFont myfont;
   connectionlistitem->GetFont(&myfont);
   myfont.SetShear(5.0);
   myfont.SetSpacing(B_CHAR_SPACING);
   connectionlistitem->SetFont(&myfont);*/
   
   //connectionlistitem->Update(aDUNview,);
   
   locationlistitem = new BOutlineListView(lst2, "location_list", B_SINGLE_SELECTION_LIST);
   locationlistitem->AddItem(locitem = new BStringItem(""));
   locationlistitem->AddUnder(locsubitem = new BStringItem(""), locitem);
   locationlistitem->Collapse(locitem); 
                  
   // Frames
   BRect tf(10,18,302,67);    
   topframe = new BBox(tf,"Connect to:",B_FOLLOW_ALL | B_WILL_DRAW | B_NAVIGABLE_JUMP, B_FANCY_BORDER);
   BRect mf(10,86,302,139);    
   middleframe = new BBox(mf,"From Location:",B_FOLLOW_ALL | B_WILL_DRAW | B_NAVIGABLE_JUMP, B_FANCY_BORDER);
   BRect bf(10,149,302,208);    
   bottomframe = new BBox(bf,"Connection",B_FOLLOW_ALL | B_WILL_DRAW | B_NAVIGABLE_JUMP, B_FANCY_BORDER);
   
   // Set Labels for Frames
   //middleframe->SetLabel("From Location:");
   bottomframe->SetLabel("Connection");
           
   // Add our Objects to the Window
   AddChild(modembutton);
   AddChild(disconnectbutton);
   AddChild(connectbutton);
//   AddChild(disablecallwaiting);
//   AddChild(dialoutprefix);
   AddChild(connectionlistitem);
   AddChild(topframe);
   AddChild(connectionmenufield); 
   AddChild(locationlistitem); 
   AddChild(middleframe);
   AddChild(locationmenufield);
   AddChild(bottomframe);
   AddChild(tvConnectionProfile);
   AddChild(tvCallWaiting);
   AddChild(tvConnection);
   AddChild(tvTimeOnline);
   AddChild(tvLIP);
   AddChild(tvLocalIPAddress);
   
   // Make Connection Menu the Focus -- otherwise it looks funny
   connectionmenufield->MakeFocus(true);	
        
   // Disable Buttons that need to be
   disconnectbutton->SetEnabled(false);
   connectbutton->SetEnabled(false);
   
   // Set Default Button
   connectbutton->MakeDefault(true);
      
   // Add the Drawing View
   AddChild(aDUNview = new DUNView(r));
}
// ------------------------------------------------------------------------------- //

//void DUNView::MouseDown(BPoint bp) {
  //BString *tmp;
//   (new BAlert("","Clicked","Okay"))->Go();
//}

// DUNWindow::~DUNWindow -- destructor
DUNWindow::~DUNWindow() {
   exit(0);
}
// ------------------------------------------------------------------------------- //

// DUNWindow::QuitRequested -- Post a message to the app to quit
bool DUNWindow::QuitRequested() {
   be_app->PostMessage(B_QUIT_REQUESTED);
   return true;
}
// ------------------------------------------------------------------------------- //

// DUNWindow::MessageReceived -- receives messages
void DUNWindow::MessageReceived (BMessage *message) {
   switch(message->what) {
   	   case BTN_MODEM:
  	   	 new ModemWindow(windowRectModem);
  	   	 break;	
       default:
         BWindow::MessageReceived(message);
         break;
   }
}
// end
