/*

DUNWindow - DialUp Networking BWindow

Authors: Sikosis (beos@gravity24hr.com)
		 Misza (misza@ihug.com.au) 

(C) 2002 OpenBeOS under MIT license

*/
/*-----WARNING MESSY CODE AHEAD :-)------*/
#include "DUNWindow.h"
#include "NewConnectionWindow.h"

// Add these to Constants file
const int DUN_WINDOW_STATE_DEFAULT = 0;  // default - closed
const int DUN_WINDOW_STATE_ONE = 1;      // 1st only
const int DUN_WINDOW_STATE_TWO = 2;      // 2nd only
const int DUN_WINDOW_STATE_THREE = 3;    // both

int DUNWindowState;
int DUNLastWindowState;

float ModemFormTopDefault = 150;
float ModemFormLeftDefault = 150;
float ModemFormWidthDefault = 281;
float ModemFormHeightDefault = 324;
BRect windowRectModem(ModemFormTopDefault,ModemFormLeftDefault,ModemFormLeftDefault+ModemFormWidthDefault,ModemFormTopDefault+ModemFormHeightDefault);

// DUNWindow -- constructor for DUNWindow Class
DUNWindow::DUNWindow(BRect frame) : BWindow (frame, "OBOS Dial-up Networking", B_TITLED_WINDOW, B_NOT_RESIZABLE |B_NOT_ZOOMABLE, 0) {
   InitWindow();
   Show();
}
// ------------------------------------------------------------------------------- //


// DUNWindow::InitWindow -- Initialization Commands here
void DUNWindow::InitWindow() {
  
   //SetStatus(0,0);//none active at beginning
   BRect r;
   r = Bounds();
   
   // Add the Drawing View
   aDUNview = new DUNView(r);
   aDUNview->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
   AddChild(aDUNview);
   
      
   // Buttons
   float ButtonTop = 217;
   float ButtonWidth = 24;
   
   BRect btn1(10,ButtonTop,83,ButtonTop+ButtonWidth);
   modembutton = new BButton(btn1,"Modem","Modem...", new BMessage(BTN_MODEM), B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE);
   BRect btn2(143,ButtonTop,218,ButtonTop+ButtonWidth);
   disconnectbutton = new BButton(btn2,"Disconnect","Disconnect", new BMessage(BTN_DISCONNECT), B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE);
   BRect btn3(230,ButtonTop,302,ButtonTop+ButtonWidth);
   connectbutton = new BButton(btn3,"Connect","Connect", new BMessage(BTN_CONNECT), B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE);
   
   connectbutton->MakeDefault(true);
   disconnectbutton->SetEnabled(false);
   
   aDUNview->AddChild(modembutton);
   aDUNview->AddChild(disconnectbutton);
   aDUNview->AddChild(connectbutton);
   
   // Connection MenuField
   BRect mfld1(17,8,175,31);
   BMenuItem *menuconnew;
   BMenuItem *menucondelete;
   
   top_frame_label = new BView(mfld1,"top_frame_label",B_FOLLOW_NONE,B_WILL_DRAW|B_NAVIGABLE);
   top_frame_label->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
  
   conmenufield = new BMenu("Click to add");
   //conmenufield = new BPopUpMenu("Click to add");
   conmenufield->AddSeparatorItem();
   conmenufield->AddItem(menuconnew = new BMenuItem("New...", new BMessage(MENU_CON_NEW)));
   menuconnew->SetTarget(be_app);
   conmenufield->AddItem(menucondelete = new BMenuItem("Delete Current", new BMessage(MENU_CON_DELETE_CURRENT)));
   menucondelete->SetEnabled(false);
   menucondelete->SetTarget(be_app);
   
   connectionmenufield = new BMenuField(top_frame_label->Bounds(),"connection_menu","Connect to:",conmenufield,B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
   connectionmenufield->SetDivider(77);	
   connectionmenufield->SetFont(be_bold_font);
   connectionmenufield->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
   
   // Location MenuField
   BRect mfld2(17,75,205,99);//60 for small
   BRect middle_rect_drp = mfld2;
   
   BMenuItem *menulochome;
   BMenuItem *menulocwork;
   BMenuItem *menulocnew;
   BMenuItem *menulocdelete;
   
   middle_frame_label = new BView(middle_rect_drp,"middle_frame_label",B_FOLLOW_NONE,B_WILL_DRAW|B_NAVIGABLE);
   middle_frame_label->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
   
   locmenufield = new BPopUpMenu("Home            ");
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
   
   locationmenufield = new BMenuField(middle_frame_label->Bounds(),"location_menu","From location:",locmenufield,B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
   locationmenufield->SetDivider(92);	
   locationmenufield->SetFont(be_bold_font);
   locationmenufield->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
   
   // Frames
   BRect tf(10,8,302,67);
   topframe = new BBox(tf,"Connect to:",B_FOLLOW_NONE | B_WILL_DRAW | B_NAVIGABLE_JUMP, B_FANCY_BORDER);
   topframe->SetLabel(top_frame_label);
   top_frame_label->AddChild(connectionmenufield);
   
   passmeon = new BStringView(BRect(24,31,284,49),"tvConnectionProfile", "<Create a connection profile to continue.>");
   passmeon->SetViewColor(255,0,0);
   
   
   passmeon2 = new DetailsView();
   
  //***********INTEGRAL TO GETTING SET STATE READY
   //passmeon2->Hide();
   topframe->AddChild(passmeon2);
   topframe->AddChild(passmeon);
   
   
   BRect THESPOT = BRect(10,37,19,45);
   
   trvconnect = new TreeView(THESPOT);
   trvconnect->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
   topframe->AddChild(trvconnect);
   
   aDUNview->AddChild(topframe);


   BRect mf(10,76,302,139); 
   middleframe = new BBox(mf,"From Location:",B_FOLLOW_NONE | B_WILL_DRAW | B_NAVIGABLE_JUMP, B_FANCY_BORDER);
   
   middleframe->SetLabel(middle_frame_label);
   middle_frame_label->AddChild(locationmenufield);

   passmeon4 = new BStringView(BRect(25,31+4,284,49+4),"tvCallWaiting","Call waiting may be enabled.");
   passmeon3 = new LocationView();
   //***********INTEGRAL TO GETTING SET STATE READY
   //passmeon3->Hide();
   
   middleframe->AddChild(passmeon3);
   middleframe->AddChild(passmeon4);
   
      BRect THESPOT2 = BRect(10,37+4,19,45+4);
   trvlocation = new TreeView(THESPOT2);
   trvlocation->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
   
   middleframe->AddChild(trvlocation);
   aDUNview->AddChild(middleframe);
     
   
    BRect bf(10,149,302,208);    
    bottomframe = new BBox(bf,"Connection",B_FOLLOW_BOTTOM | B_WILL_DRAW | B_NAVIGABLE_JUMP, B_FANCY_BORDER);
    bottomframe->SetLabel("Connection");
   
    // Displays - No Connection
    BRect NCLocation(10,18,80,30);
    svNoConnect = new BStringView(NCLocation,"svNoConnect","No Connection");
   
    // Displays - Text Label of Local IP address
    BRect LocalIPAddressLocation(NCLocation.left,35,100,47);
	svLocalIPAddress = new BStringView(LocalIPAddressLocation,"svLocalIPAddress","Local IP address:");
	
	// Display - Time Online ie. 00:00:00
	BRect TOLocation(238,NCLocation.top,288,NCLocation.bottom);
	svTimeOnline = new BStringView(TOLocation,"svTimeOnline","00:00:00");
	
	// Display - Local IP address ie. None -- might find some code to get the current ip address and 
	//                                put that in here instead ... hrmmm ;)
    BRect LIPLocation (253,LocalIPAddressLocation.top,283,LocalIPAddressLocation.bottom);
    svLIP = new BStringView(LIPLocation,"svLIP","None");
	
	// Add to Bottom Frame 	  
    bottomframe->AddChild(svNoConnect);
    bottomframe->AddChild(svLocalIPAddress);
    bottomframe->AddChild(svTimeOnline);
    bottomframe->AddChild(svLIP);
  
    aDUNview->AddChild(bottomframe);
   
	// Load User Settings
	BPath path;
  	find_directory(B_USER_SETTINGS_DIRECTORY,&path);
  	path.Append("DUN/OBOS_DUN_settings",true);
  	BFile file(path.Path(),B_READ_ONLY);
	BMessage msg;
  	msg.Unflatten(&file);
	LoadSettings (&msg);
}
// ------------------------------------------------------------------------------- //


// DUNWindow::~DUNWindow -- destructor
DUNWindow::~DUNWindow() 
{
   exit(0);
}
// ------------------------------------------------------------------------------- //


void DUNWindow::DUNResizeHandler()
{	//post: window has been set to last state :-)
	switch (DUNWindowState)
	{	
		case DUN_WINDOW_STATE_DEFAULT:
		{
			SetStatus(0,0);
			trvconnect->SetTreeViewStatus(false);
			trvlocation->SetTreeViewStatus(false);
			ResizeTo(312,250);
			passmeon2->Hide();
			passmeon3->Hide();

		}	break;
		
		case DUN_WINDOW_STATE_ONE:
		{
			ResizeTo(312,250);
			SetStatus(0,0);
			trvconnect->SetTreeViewStatus(true);
			trvlocation->SetTreeViewStatus(false);
			DoResizeMagic();

		}	break;
		case DUN_WINDOW_STATE_TWO:
		{
			ResizeTo(312,250);
			SetStatus(0,0);
			trvconnect->SetTreeViewStatus(false);
			trvlocation->SetTreeViewStatus(true);
			DoResizeMagic();
		}	break;
		case DUN_WINDOW_STATE_THREE:
		{
			ResizeTo(312,250);
			SetStatus(0,0);
			trvconnect->SetTreeViewStatus(true);
			trvlocation->SetTreeViewStatus(true);
			DoResizeMagic();
		}	break;			
	}
}
//------------------------------------------------------//


// DUNWindow::LoadSettings -- Loads your current DUN settings
void DUNWindow::LoadSettings(BMessage *msg)
{
	BRect frame;
	if (B_OK == msg->FindRect("windowframe",&frame)) {
		MoveTo(frame.left,frame.top);
		ResizeTo(frame.right-frame.left,frame.bottom-frame.top);
	
	}
	//load a last mode thingy to prevent cumulative resizing
	int16   LoadedLastWindowState;
	if (B_OK == msg->FindInt16("lastwindowstate",&LoadedLastWindowState)) {
		DUNLastWindowState = LoadedLastWindowState;
		cout << "From Settings: " << DUNLastWindowState << endl; 
	}
	else
	DUNLastWindowState = DUN_WINDOW_STATE_DEFAULT;
	
	
	int16   LoadedWindowState;
	
	if (B_OK == msg->FindInt16("windowstate",&LoadedWindowState)) {
		DUNWindowState = LoadedWindowState;
		cout << "From Settings: " << DUNWindowState << endl; 
		
		DUNResizeHandler();
	}
	else
	{
	DUNWindowState = DUN_WINDOW_STATE_DEFAULT;//setup window state as def
	DUNResizeHandler();
	}
		
	//**********---SETTINGS CONFUSE ME! :P **********//
	/*int16 _x,_y;
	if(B_OK == msg->FindInt16("rx",0,&_x) && B_OK == msg->FindInt16("ry",0,&_y))
	{
		SetStatus(_x,_y);
	//		cout << "cool" << endl;
	}*/
	
	/*int16 stat1,stat2;
	if(B_OK == msg->FindInt16("status1",0,&stat1) && B_OK == msg->FindInt16("status2",0,&stat2))
	{
		if(stat1)
			TestBox1->SetValue(B_CONTROL_ON);
		else if(!stat1)
			TestBox1->SetValue(B_CONTROL_OFF);
		else if(stat2)
			TestBox1->SetValue(B_CONTROL_ON);
		else if(!stat2)
			TestBox2->SetValue(B_CONTROL_OFF);
	}*/

}
// ------------------------------------------------------------------------------- //

// DUNWindow::SaveSettings -- Saves the Users DUN settings
void DUNWindow::SaveSettings()
{
	BMessage msg;
	msg.AddRect("windowframe",Frame());
	
	msg.AddInt16("windowstate",DUNWindowState);
	DUNLastWindowState = DUNWindowState;
	msg.AddInt16("lastwindowstate",DUNLastWindowState);
	
	/*msg.AddInt16("rx",last.x);
	msg.AddInt16("ry",last.y);
	

	if(TestBox1->Value() == B_CONTROL_ON)
	msg.AddInt16("status1",1);
	else
	msg.AddInt16("status1",0);
	
	if(TestBox2->Value() == B_CONTROL_ON)
	msg.AddInt16("status2",1);
	else
	msg.AddInt16("status2",0);
	*/
	
	/*int selection=ListView1->CurrentSelection(0);
	if (selection>=0)
		msg.AddString("modulename", ((BStringItem *)(ListView1->ItemAt(selection)))->Text()); 
	msg.what=POPULATE;	*/
	
    BPath path;
    status_t result = find_directory(B_USER_SETTINGS_DIRECTORY,&path);
    if (result == B_OK)
    {
	    path.Append("DUN/OBOS_DUN_settings",true);
    	BFile file(path.Path(),B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	    msg.Flatten(&file);
	}    
}
// ------------------------------------------------------------------------------- //


// DUNWindow::QuitRequested -- Post a message to the app to quit
bool DUNWindow::QuitRequested() {
	SaveSettings();
    be_app->PostMessage(B_QUIT_REQUESTED);
    return true;
}
// ------------------------------------------------------------------------------- //


// DUNWindow::MessageReceived -- receives messages
void DUNWindow::MessageReceived (BMessage *message)
{	
	switch(message->what) 
	{
		case BTN_CONNECT:	
		{
   	    	//disconnectbutton->SetEnabled(true);
   	    	//connectbutton->SetEnabled(false);
			//BAlert *errormsg = new BAlert("errormsg", "Chaos reigns within.\nReflect, repent, and retry.\nConnect shall return.", "   Haiku Error ;)   " , NULL, NULL, B_WIDTH_FROM_WIDEST, B_IDEA_ALERT);
		    //errormsg->SetShortcut(0, B_ESCAPE);
    		//errormsg->Go();
    		//debug
  	   		ptrNewConnectionWindow = new NewConnectionWindow(BRect(0,0,260,73));
		}	break;
		case BTN_DISCONNECT:
		{
   	    	disconnectbutton->SetEnabled(false);
   	    	connectbutton->SetEnabled(true);
   	    	BAlert *errormsg = new BAlert("errormsg", "Short little poem\nReally beats the hell out of\nAn error message", "   I Love Poetry   ", NULL , NULL, B_WIDTH_FROM_WIDEST, B_WARNING_ALERT);   	   		
   	    	errormsg->SetShortcut(0, B_ESCAPE);
   	    	errormsg->Go();
   	    	
	   	}    break;
	   	case MENU_CON_NEW:
	   	{
	   		ptrNewConnectionWindow = new NewConnectionWindow(BRect(0,0,250,50));
	   	}	break;
   	    case BTN_MODEM:
  	   	{	
  	   		modemWindow = new ModemWindow(windowRectModem);
  	   		
	   	}    break;	
  	    case BTN_SETTINGS:
  	   		settingsWindow = new SettingsWindow(BRect(367.0, 268.0, 657.0, 500.0));
 	 	    break;
        default:
			BWindow::MessageReceived(message);
	        break;
	}
}
// ------------------------------------------------------------------------------- //


// DUNWindow::InvalidateAll
void DUNWindow::InvalidateAll()
{	//redraws the top and middle BBoxen, so that DoResizeMagic() 
	//(InvalidateAll() is called by DoResizeMagic()) works smoothly :D
	topframe->Invalidate();
	middleframe->Invalidate();
}
// ------------------------------------------------------------------------------- //


// DUNWindow::DoResizeMagic -- this magical function handles the resizing of the main window,
// NOTE: it is quite messy & confusing only the brave may try to understand this code
void DUNWindow::DoResizeMagic()
{
	if(trvconnect->TreeViewStatus() == 1 && trvlocation->TreeViewStatus() == 1)
	{	//BOTH ON	
  	   	if(last.x == 1 && last.y == 0)
  	   	{	//pre: 1,0
  	   		//post: 1,1
  	   		topframe->SetResizingMode(B_FOLLOW_TOP);
  	   		middleframe->SetResizingMode(B_FOLLOW_TOP_BOTTOM);
  	   		middle_frame_label->SetResizingMode(B_FOLLOW_TOP);
  	   				
  	   		if(!(passmeon->IsHidden())) passmeon->Hide();
  	   		if(!(passmeon4->IsHidden()))passmeon4->Hide();
  	   		if((passmeon2->IsHidden()))passmeon2->Show();
  	   		if((passmeon3->IsHidden()))passmeon3->Show();
  	   				
  	   		passmeon->SetResizingMode(B_FOLLOW_TOP);
  	   		passmeon4->SetResizingMode(B_FOLLOW_TOP);
  	   		passmeon2->SetResizingMode(B_FOLLOW_TOP);
  	   		passmeon3->SetResizingMode(B_FOLLOW_TOP);
  	   				
  	   		ResizeBy(0,31);	   			
  	   	}
  	   			
  	   	if(last.x == 0 && last.y == 1)
  	   	{	//pre:  0,1
  	   		//post: 1,1
  	   		topframe->SetResizingMode(B_FOLLOW_TOP_BOTTOM);
  	   		middleframe->SetResizingMode(B_FOLLOW_BOTTOM);
  	   		middle_frame_label->SetResizingMode(B_FOLLOW_BOTTOM);

  	   		if(!(passmeon->IsHidden())) passmeon->Hide();
  	   		if(!(passmeon4->IsHidden()))passmeon4->Hide();
  	   		if((passmeon2->IsHidden()))passmeon2->Show();
  	   		if((passmeon3->IsHidden()))passmeon3->Show();
  	   		
  	   		passmeon->SetResizingMode(B_FOLLOW_TOP);
  	   		passmeon4->SetResizingMode(B_FOLLOW_TOP);
  	   		passmeon2->SetResizingMode(B_FOLLOW_TOP);
  	   		passmeon3->SetResizingMode(B_FOLLOW_TOP);
  	   		ResizeBy(0,100);
  	   	}
  	   	
  	   	if(last.x == 0 && last.y == 0)
  	   	{	//if loading from settings 3
  	   	  	//pre: 0,0
  	   	  	//post: 1,1
  	   	  	topframe->SetResizingMode(B_FOLLOW_TOP);
  	   		middleframe->SetResizingMode(B_FOLLOW_TOP_BOTTOM);
  	   		middle_frame_label->SetResizingMode(B_FOLLOW_TOP);
  	   	  	
  	   	
  	   		passmeon->SetResizingMode(B_FOLLOW_TOP);
  	   		passmeon4->SetResizingMode(B_FOLLOW_TOP);
  	   		passmeon2->SetResizingMode(B_FOLLOW_TOP);
  	   		passmeon3->SetResizingMode(B_FOLLOW_TOP);
  	   		
  	   		ResizeBy(0,31);
  	   		
  	   		topframe->SetResizingMode(B_FOLLOW_TOP_BOTTOM);
  	   		middleframe->SetResizingMode(B_FOLLOW_BOTTOM);
  	   		middle_frame_label->SetResizingMode(B_FOLLOW_BOTTOM);
  	   	
  	   		passmeon->SetResizingMode(B_FOLLOW_TOP);
  	   		passmeon4->SetResizingMode(B_FOLLOW_TOP);
  	   		passmeon2->SetResizingMode(B_FOLLOW_TOP);
  	   		passmeon3->SetResizingMode(B_FOLLOW_TOP);
  		   	
  		   	ResizeBy(0,100);
			passmeon->Hide();
  	   		passmeon4->Hide();
  	   		DUNLastWindowState = 4;//so we don't keep doing this :P
			}
		DUNWindowState = DUN_WINDOW_STATE_THREE;
  	   	InvalidateAll();
  	   	SetStatus(1,1);
	}
  	   		
  	else if(trvconnect->TreeViewStatus() == 0 && trvlocation->TreeViewStatus() == 0)
	{	//both off

  		if(last.x == 1 && last.y == 0)
  	   		{	//pre: 1,0
  	   			//post: 0,0
  	   			
  	   			topframe->SetResizingMode(B_FOLLOW_TOP_BOTTOM);
  	   			middleframe->SetResizingMode(B_FOLLOW_BOTTOM);
  	   			middle_frame_label->SetResizingMode(B_FOLLOW_BOTTOM);
  	   			
  	   			if(!(passmeon3->IsHidden())) passmeon3->Hide();
  	   			if(!(passmeon2->IsHidden())) passmeon2->Hide();
  	   			if((passmeon->IsHidden()))passmeon->Show();
  	   			if((passmeon4->IsHidden()))passmeon4->Show();
  	   			
 	   			passmeon->SetResizingMode(B_FOLLOW_TOP);
 	   			passmeon4->SetResizingMode(B_FOLLOW_TOP);
  	   			passmeon2->SetResizingMode(B_FOLLOW_TOP);
  	   			passmeon3->SetResizingMode(B_FOLLOW_TOP);
  	   				
  	   			ResizeBy(0,-100);
  	   	 	}
  	   			
		if(last.x == 0 && last.y == 1)
  	   	{	//pre: 0,1
  	   		//post:0,0
  	   		topframe->SetResizingMode(B_FOLLOW_TOP);
  	   		middleframe->SetResizingMode(B_FOLLOW_TOP_BOTTOM);
  	   		middle_frame_label->SetResizingMode(B_FOLLOW_TOP);
  	   				
  	   		if(!(passmeon3->IsHidden()))passmeon3->Hide();
  	   		if(!(passmeon2->IsHidden()))passmeon2->Hide();
  	   		if((passmeon->IsHidden()))passmeon->Show();
  	   		if((passmeon4->IsHidden()))passmeon4->Show();
  	   				
  	   		passmeon->SetResizingMode(B_FOLLOW_TOP);
  	   		passmeon4->SetResizingMode(B_FOLLOW_TOP);
  	   		passmeon2->SetResizingMode(B_FOLLOW_TOP);
  	   		passmeon3->SetResizingMode(B_FOLLOW_TOP);
  	   				  	   				
  	   		ResizeBy(0,-31);
  	   	}
  	   	
  	   	DUNWindowState = DUN_WINDOW_STATE_DEFAULT;	
 		InvalidateAll();
		SetStatus(0,0);
	}
  	   		
  	   		else if(trvconnect->TreeViewStatus() == 1 && trvlocation->TreeViewStatus() == 0)
  	   		{	//1,0
  	   			if(last.x == 0 && last.y == 0)
  	   			{	//pre: 0,0
  	   				//post: 1,0
  	   				topframe->SetResizingMode(B_FOLLOW_TOP_BOTTOM);
  	   				middleframe->SetResizingMode(B_FOLLOW_BOTTOM);
  	   				middle_frame_label->SetResizingMode(B_FOLLOW_BOTTOM);
  	   			  	
  	   			  	if(DUNLastWindowState == DUN_WINDOW_STATE_ONE)
  	   			  	{
  	   			  	passmeon->Hide();
  	   				passmeon3->Hide();
					DUNLastWindowState = 4;//so we don't keep doing this :P
					}
					else
					{
  	   			  	if(!(passmeon->IsHidden())) passmeon->Hide();
  	   				if(!(passmeon3->IsHidden()))passmeon3->Hide();
  	   				if((passmeon2->IsHidden()))passmeon2->Show();
  	   				if((passmeon4->IsHidden()))passmeon4->Show();
					}
  	   				passmeon->SetResizingMode(B_FOLLOW_TOP);
  	   				passmeon4->SetResizingMode(B_FOLLOW_TOP);
  	   				passmeon2->SetResizingMode(B_FOLLOW_TOP);
  	   				passmeon3->SetResizingMode(B_FOLLOW_TOP);
  	   				
  	   				ResizeBy(0,100); 	   							
  	   			}
  	   			
  	   			if(last.x == 1 && last.y == 1)
  	   			{	//pre: 1,1
  	   				//post:	1,0
  	   				topframe->SetResizingMode(B_FOLLOW_TOP);
  	   				middleframe->SetResizingMode(B_FOLLOW_TOP_BOTTOM);
  	   				middle_frame_label->SetResizingMode(B_FOLLOW_TOP);
  	   			  	
  	   			  	if(!(passmeon->IsHidden())) passmeon->Hide();
  	   				if(!(passmeon3->IsHidden()))passmeon3->Hide();
  	   				if((passmeon2->IsHidden()))passmeon2->Show();
  	   				if((passmeon4->IsHidden()))passmeon4->Show();
  	   				
  	   				passmeon->SetResizingMode(B_FOLLOW_TOP);
  	   				passmeon4->SetResizingMode(B_FOLLOW_TOP);
  	   				passmeon2->SetResizingMode(B_FOLLOW_TOP);
  	   				passmeon3->SetResizingMode(B_FOLLOW_TOP);
  	   				
  	   				ResizeBy(0,-31);  	
  	   			}
  	   			
  	   			
  	   			DUNWindowState = DUN_WINDOW_STATE_ONE;
  	   			InvalidateAll();
  	   			SetStatus(1,0);
  	   		}
  	   		else if(trvconnect->TreeViewStatus() == 0 && trvlocation->TreeViewStatus() == 1)
  	   		{	//0,1
  	   			
  	   			if(last.x == 0 && last.y == 0)
  	   			{	//pre:0,0
  	   				//post: 0,1
  	   				topframe->SetResizingMode(B_FOLLOW_TOP);
  	   				middleframe->SetResizingMode(B_FOLLOW_TOP_BOTTOM);
  	   				middle_frame_label->SetResizingMode(B_FOLLOW_TOP);
  	   			  	
  	   			  	if(DUNLastWindowState == DUN_WINDOW_STATE_TWO)
  	   			  	{
  	   			  	passmeon4->Hide();
  	   				passmeon2->Hide(); 	   				
  					DUNLastWindowState = 4;//so we don't keep doing this :P
  					}
  					else
  					{
  	   			  	if(!(passmeon4->IsHidden())) passmeon4->Hide();
  	   				if(!(passmeon2->IsHidden()))passmeon2->Hide();
  	   				if((passmeon3->IsHidden()))passmeon3->Show();
  	   				if((passmeon->IsHidden()))passmeon->Show();  	   				
  					}
  	   				
  	   				passmeon->SetResizingMode(B_FOLLOW_TOP);
  	   				passmeon4->SetResizingMode(B_FOLLOW_TOP);
  	   				passmeon2->SetResizingMode(B_FOLLOW_TOP);
  	   				passmeon3->SetResizingMode(B_FOLLOW_TOP);

  	   				ResizeBy(0,31);  	   		  	   			
  	   			
  	   			}
  	   			if(last.x == 1 && last.y == 1)
  	   			{	//pre: 1,1
  	   				//post: 0,1
   	   				topframe->SetResizingMode(B_FOLLOW_TOP_BOTTOM);
  	   				middleframe->SetResizingMode(B_FOLLOW_BOTTOM);
  	   				middle_frame_label->SetResizingMode(B_FOLLOW_BOTTOM);
 
  	   			  	if(!(passmeon4->IsHidden()))passmeon4->Hide();
  	   				if(!(passmeon2->IsHidden()))passmeon2->Hide();
  	   				if((passmeon3->IsHidden()))passmeon3->Show();
  	   				if((passmeon->IsHidden()))passmeon->Show();  	   				
  	   				passmeon->SetResizingMode(B_FOLLOW_TOP);
  	   				passmeon4->SetResizingMode(B_FOLLOW_TOP);
  	   				passmeon2->SetResizingMode(B_FOLLOW_TOP);
  	   				passmeon3->SetResizingMode(B_FOLLOW_TOP);
  	   				
					ResizeBy(0,-100);
					
  	   			}
  	   				
  	   			DUNWindowState = DUN_WINDOW_STATE_TWO;
  	   			InvalidateAll();
  	   			SetStatus(0,1);
  	   		}
}
// ------------------------------------------------------------------------------- //
// DUNWindow::SetStatus -- Integral to how DoResizeMagic Works! :-)
void DUNWindow::SetStatus(int m,int n)
{
	last.x = m;
	last.y = n;
}
// ------------------------------------------------------------------------------- //


// end
