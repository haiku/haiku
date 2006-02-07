#include "NetworkWindow.h"
#include "NetListView.h"
#include <ScrollView.h>
#include <StringView.h>
#include <Screen.h>
#include <malloc.h>
#include <stdlib.h>
#include <TextControl.h>
#include <Alert.h>
#include <Application.h>
#include <stdio.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>

#include "InterfaceWin.h"
#include "BackupWindow.h"
#include "LoginInfo.h"

#define NETWORK_WINDOW_RIGHT 435
#define NETWORK_WINDOW_BOTTOM 309

class RestartWindow : public BWindow
{
public:
	RestartWindow();
	
	BView		*fView;
	BStringView *fRestarting;
	BStringView *fWhatsHappening;
};


RestartWindow::RestartWindow()
  : BWindow(BRect(400,300,650,350),"Restart Networking",B_MODAL_WINDOW,B_NOT_ANCHORED_ON_ACTIVATE | B_NOT_RESIZABLE,B_CURRENT_WORKSPACE)
{
	fView = new BView(Bounds(),"Restart_View",B_FOLLOW_ALL_SIDES,B_WILL_DRAW);
	
	fView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fRestarting     = new BStringView(BRect(60,5,200,25),"Restarting_Text_1","Restarting Networking");
	fWhatsHappening = new BStringView(BRect(60,30,200,50),"Restarting_Text_2","Please Wait..."); 
	
	BFont font(be_bold_font);
	
	fRestarting->SetFont(&font);
	fWhatsHappening->SetFont(&font);
	
	fView->AddChild(fRestarting);
	fView->AddChild(fWhatsHappening);

	AddChild(fView);		
}

NetworkWindow::NetworkWindow()
  : BWindow(BRect(0, 0, NETWORK_WINDOW_RIGHT, NETWORK_WINDOW_BOTTOM),"Network",
			B_TITLED_WINDOW,B_NOT_ANCHORED_ON_ACTIVATE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE,
			B_CURRENT_WORKSPACE) 
{
	// if(!Network_settings)
	// { ...
	BScreen screen;
	
	BRect screenframe = screen.Frame();
	float screenx, screeny;
	screenx = ((screenframe.right + 1) / 2) - (NETWORK_WINDOW_RIGHT / 2) - 1;
	screeny = ((screenframe.bottom + 1) / 2) - (NETWORK_WINDOW_BOTTOM / 2) - 1;
	MoveTo(screenx, screeny);
	// }
	// else
	// {
	// ...
	// }
	
	BRect bounds = Bounds();
	fView    = new BView(bounds,"View",B_FOLLOW_ALL_SIDES,B_WILL_DRAW);
	
	fRestart = new BButton(BRect(11,274,122,298),"Restart_Networking","Restart Networking",new BMessage(kRestart_Networking_M));
	fRevert  = new BButton(BRect(145,274,220,298),"Revert","Revert",new BMessage(kRevert_M));
	fSave	 = new BButton(BRect(349,274,424,297),"Save","Save",new BMessage(kSave_M));
	fSave->MakeDefault(true);
	fSave->SetEnabled(false);
	fRevert->SetEnabled(false);
	
	bounds.bottom = Bounds().Height();
	bounds.top += 10;
	bounds.right = Bounds().Width();
	fTabView = new BTabView(bounds,"tab_view", B_WIDTH_FROM_WIDEST);
	
	fIdentity       = new BTab();
	bounds.bottom -= fTabView->TabHeight();
	fIdentityView  = new BView(bounds,"Identity",B_FOLLOW_ALL_SIDES,B_WILL_DRAW);
	fServicesView  = new BView(bounds,"Services",B_FOLLOW_ALL_SIDES,B_WILL_DRAW);
	
	bounds.top -= 10;
	bounds.bottom -= 209;
	bounds.left += 11;
	bounds.right  -= 12; 
	fNames 	   	   = new BBox(bounds,"Names",B_FOLLOW_LEFT | B_FOLLOW_TOP,B_WILL_DRAW | B_FRAME_EVENTS);
	
	fDomain     	= new BTextControl(BRect(15,19,195,50),"Domain_Name","Domain name:",NULL,new BMessage(kSOMETHING_HAS_CHANGED_M));  
	fHost			= new BTextControl(BRect(15,49,195,90),"Host_Name","Host name:",NULL,new BMessage(kSOMETHING_HAS_CHANGED_M));   
	fPrimaryDNS	= new BTextControl(BRect(209,19,399,45),"Primary_DNS","Primary DNS:",NULL,new BMessage(kSOMETHING_HAS_CHANGED_M));  
	fSecondaryDNS	= new BTextControl(BRect(209,49,399,90),"Secondary_DNS","Secondary DNS:",NULL,new BMessage(kSOMETHING_HAS_CHANGED_M));   
	
	bounds.top     = bounds.bottom + 12;
	bounds.bottom  = 208; 
	fInterfaces     = new BBox(bounds,"Network_Interfaces",B_FOLLOW_LEFT | B_FOLLOW_TOP,B_WILL_DRAW | B_FRAME_EVENTS); 
	
	fSettings	= new BButton(BRect(325,16,400,37),"Settings","Settings...",new BMessage(kSettings_M));
	fClear		= new BButton(BRect(325,45,400,67),"Clear","Clear",new BMessage(kClear_M));
	fCustom		= new BButton(BRect(325,83,400,107),"Custom","Custom...",new BMessage(kCustom_M));
	//fSettings	->SetEnabled(false);
	//fClear		->SetEnabled(false);
	
	bounds							 = fInterfaces->Bounds();
	bounds.InsetBy(20,20);
	bounds.top						 ++;
	bounds.bottom					 += 6;
	bounds.right					 -= 94;
	bounds.left						 -= 5;  
	fInterfacesList 			 = new NetListView(bounds,"Interfaces_List");	
	BScrollView *interfaces_scroller = new BScrollView("Interfaces_Scroller",fInterfacesList, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, 0, 1, B_FANCY_BORDER);	
	
	fServices       = new BTab();
	fFTPServer	   = new BCheckBox(BRect(10,20,120,40),"FTP_Server","FTP Server",new BMessage(kSOMETHING_HAS_CHANGED_M));
	fTelnetServer  = new BCheckBox(BRect(10,50,120,70),"Telnet_Server","Telnet Server",new BMessage(kSOMETHING_HAS_CHANGED_M));
	fAppleTalk	   = new BCheckBox(BRect(10,110,120,130),"Apple_Talk","Apple Talk",new BMessage(kSOMETHING_HAS_CHANGED_M));
	fIPForwarding  = new BCheckBox(BRect(10,140,120,160),"IP_Forwarding","IP Forwarding",new BMessage(kSOMETHING_HAS_CHANGED_M));
	fLoginInfo	   = new BButton(BRect(10,80,120,100),"Login_Info","Login Info...",new BMessage(kLogin_Info_M));
	
	fConfigurations = new BBox(BRect(200,10,460,210),"Configurations",B_FOLLOW_LEFT | B_FOLLOW_TOP,B_WILL_DRAW | B_FRAME_EVENTS); 							
	
	bounds = fConfigurations->Bounds();
	bounds.InsetBy(20,20);
	bounds.right -= 80;
	fConfigurationsList 			 = new NetListView(bounds,"Configurations_List");	
	BScrollView *configurations_scroller = new BScrollView("Configurations_Scroller",fConfigurationsList);	
	
	fBackup   = new BButton(BRect(170,17,250,37),"Backup","Backup",new BMessage(kBackup_M));
	fRestore  = new BButton(BRect(170,47,250,67),"Restore","Restore",new BMessage(kRestore_M));
	fRemove   = new BButton(BRect(170,87,250,107),"Delete","Delete",new BMessage(kDelete_M));
	fRestore->SetEnabled(false);
	fRemove->SetEnabled(false);
	
	BBox *hr = new BBox(BRect(1,261,Bounds().Width()-1,262), "horizontal_rule");
	BBox *vr = new BBox(BRect(133, 274, 134, 299), "vertical_rule");
	
	fDomain->SetDivider(70);
	fDomain->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	fHost->SetDivider(70);
	fHost->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	fPrimaryDNS->SetDivider(80);
	fPrimaryDNS->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	fSecondaryDNS->SetDivider(80);
	fSecondaryDNS->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);			
	
	fNames			->SetLabel("Names");
	fInterfaces		->SetLabel("Network Interfaces");
	fConfigurations	->SetLabel("Configurations");
	fIdentityView	->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fServicesView	->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fTabView		->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));	
	fView			->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	// Casts an AddEverything 
	fTabView->AddTab(fIdentityView,fIdentity);
	fTabView->AddTab(fServicesView,fServices);
		
	fNames->AddChild(fDomain);
	fNames->AddChild(fHost);
	fNames->AddChild(fPrimaryDNS);
	fNames->AddChild(fSecondaryDNS);
	
	fInterfaces->AddChild(interfaces_scroller);
	fInterfaces->AddChild(fSettings);
	fInterfaces->AddChild(fClear);
	fInterfaces->AddChild(fCustom);
	
	fIdentityView->AddChild(fNames);
	fIdentityView->AddChild(fInterfaces);
	
	fServicesView->AddChild(fFTPServer);
	fServicesView->AddChild(fTelnetServer);
	fServicesView->AddChild(fIPForwarding);
	fServicesView->AddChild(fAppleTalk);
	fServicesView->AddChild(fLoginInfo);
	fServicesView->AddChild(fConfigurations);
	
	fConfigurations->AddChild(configurations_scroller);
	fConfigurations->AddChild(fBackup);
	fConfigurations->AddChild(fRestore);
	fConfigurations->AddChild(fRemove);
	
	fView->AddChild(fTabView);
	fView->AddChild(fRestart);
	fView->AddChild(fRevert);	
	fView->AddChild(fSave);
	fView->AddChild(hr);
	fView->AddChild(vr);
	AddChild(fView);	
}

void NetworkWindow::MessageReceived(BMessage *message)
{
	switch (message->what){
		case kRestart_Networking_M:{
		
			RestartWindow *restartWindow = new RestartWindow();
			restartWindow->Show();
					
			BMessenger messenger("application/x-vnd.Be-NETS");
			if (messenger.IsValid() == true)
				messenger.SendMessage(B_QUIT_REQUESTED);
							
			const char **arg_v;
			int32 argc;
			thread_id thread;
			status_t return_value;
			
			argc  = 1;
			arg_v = (const char **)malloc(sizeof(char *) * (argc + 1));
			
			arg_v[0] = strdup("/boot/beos/system/servers/net_server");
			arg_v[1] = NULL;
			
			thread = load_image(argc,arg_v,(const char **)environ); 
			
			while (--argc >= 0)
				free((void *)arg_v[argc]);
			free (arg_v);
			
			return_value = resume_thread(thread);			
			
			restartWindow->Quit();			
			break;				
		}
		case kRevert_M:{
			GetEverything();
			fRevert->SetEnabled(false);
			fSave->SetEnabled(false);			
			break;
		}
		case kSave_M:{
			BackupWin *bak = new BackupWin(Bounds());
			bak->fParentWindow = dynamic_cast <NetworkWindow *> (fView->Window());
			bak->SaveBackup();
			bak->Quit();
			fRevert->SetEnabled(false);
			fSave->SetEnabled(false);			
			break;
		}
		case kSettings_M:{
			
			BRect r(Frame());
			r.right = r.left + 325;
			r.bottom = r.top + 225;
			
			NetListItem *selection = dynamic_cast <NetListItem *> (fInterfacesList->ItemAt(fInterfacesList->CurrentSelection()));
			if(!selection)
				break;
			
			InterfaceWin *win = new InterfaceWin(r,selection->fData);
			r=win->Frame();
			win->MoveBy((Frame().Width()-r.Width())/2,
						(Frame().Height()-r.Height())/2);
			
			win->fParentWindow = dynamic_cast <NetworkWindow *> (fView->Window());
			
			if (selection->fData.fEnabled)
				win->fEnabled->SetValue(1);
			
			if (selection->fData.fDHCP)
				win->fDHCP->SetValue(1);
			else
				win->fManual->SetValue(1);		
			
			win->fName->SetText(selection->fData.fPrettyName.String());		
			win->fIPAddress->SetText(selection->fData.fIPAddress.String());
			win->fNetMask->SetText(selection->fData.fNetMask.String());
			
			win->fChanged = false;
			
			win->fIPAddress->SetTarget(win);
			win->fNetMask->SetTarget(win);
			win->fGateway->SetTarget(win); 
			
			win->Show();
			break;
		}
		case kClear_M:{
			break;
		}
		case kCustom_M:{
			break;
		}
		case kLogin_Info_M:{
			LoginInfo *login_info_pop = new LoginInfo();
			login_info_pop->fParentWindow = dynamic_cast <NetworkWindow *> (fView->Window());
			login_info_pop->fName->SetText(fusername.String());
			login_info_pop->fName->MakeFocus();
			login_info_pop->Show();
			break;
		}
		case kBackup_M:{
			
			BRect r(Frame());
			r.right = r.left + 260;
			r.bottom = r.top + 77;
			r.OffsetBy( (Frame().Width()-260)/2, (Frame().Height()-77)/2);
			BackupWin *backup_pop = new BackupWin(r);
			backup_pop->fParentWindow = dynamic_cast <NetworkWindow *> (fView->Window());
			backup_pop->Show();
			break;
		}
		case kRestore_M:{
			BStringItem *string_item = dynamic_cast <BStringItem *> (fConfigurationsList->ItemAt(fConfigurationsList->CurrentSelection()));
			GetEverything(string_item->Text());
			PostMessage(kSOMETHING_HAS_CHANGED_M);
			break;
		}
		case kDelete_M:{
			DeleteConfigFile();
			LoadConfigEntries();
			fConfigurationsList->SelectionChanged();
			break;
		}	
		case kSOMETHING_HAS_CHANGED_M:{
			fRevert->SetEnabled(true);
			fSave->SetEnabled(true);	
			break;	
		}
	}
}

bool NetworkWindow::QuitRequested()
{
	if (fSave->IsEnabled() == true){
		BAlert *alert = new BAlert("Save Info Alert","Save changes before quitting ?","Don't Save","Cancel","Save",B_WIDTH_FROM_WIDEST,B_INFO_ALERT);
		int32 result = alert->Go();
		switch (result){
			case 0:{
				// Don't Save
				break;
			}
			case 1:{
				// Cancel
				return false;
				break;				
			}
			case 2:{
				// Save
				BackupWin *sav = new BackupWin(Bounds());
				sav->fParentWindow = dynamic_cast <NetworkWindow *> (fView->Window());
				sav->SaveBackup();
				sav->Quit();	
				break;
			}
			default:
				break;
		}
	}
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void NetworkWindow::DeleteConfigFile()
{
	BAlert *alert = new BAlert("Alert","Really delete networking configuration?",
								"Delete","Cancel");
	int32 result = alert->Go();
	if (result == 0) {
		
		BStringItem *item = dynamic_cast <BStringItem *> (fConfigurationsList->ItemAt(fConfigurationsList->CurrentSelection()));
	
		BString path;
		path = "/boot/home/config/settings/network.";
		path << item->Text();

		BEntry entry(path.String());
		entry.Remove();
	}						
}

void NetworkWindow::LoadConfigEntries()
{
	fConfigurationsList->MakeEmpty();
	
	BDirectory settings("/boot/home/config/settings/");
	BEntry entry;

	if (settings.InitCheck() == B_OK) {
	
		while (settings.GetNextEntry(&entry) == B_OK) {
			
			// All the config files start with "network." so we only compare the 
			// first 8 characters
			char buffer[9];
			entry.GetName(buffer);
			buffer[8]='\0';

			if (strcmp(buffer,"network.") == 0) {
			
				BString string;
				char name[B_FILE_NAME_LENGTH];
				entry.GetName(name);          
				string << name;
				string = string.Remove(0,8);				
				
				BStringItem *item = new BStringItem(string.String());
				fConfigurationsList->AddItem(item);
				
			}
		}
	}
}

void NetworkWindow::GetEverything(const char *bakname)
{
/*
// The configuration is saved in the /boot/home/config/settings/network
// text file so the config is static. Maybe we could switch to dynamic by
// directly asking the net_server how things are
	
	if (bakname == NULL &&
	    BEntry("/boot/home/config/settings/network").Exists() == false) {
	    
		BAlert *alert = new BAlert("Network Alert",
		"Your network config file doesn't exist.\n"
		"It should be located at/boot/home/config/settings/network ","OK");
		alert->Go();
	}
	else {
		// This function being used when pressing "Revert" so first we need to
		// empty things before setting the new values
		Lock();
		fInterfacesList->MakeEmpty();	
		fFTPServer	  ->SetValue(0);
		fTelnetServer ->SetValue(0);
		fAppleTalk	  ->SetValue(0);
		fIPForwarding ->SetValue(0);
		Unlock();
		
		BString path_S = "/boot/home/config/settings/network";
		if (bakname != NULL) path_S<<"."<<bakname;
											
    	size_t s = 0;
    	size_t i = 0;
	
		FILE *cfg_file = fopen(path_S.String(),"r");   
		
		while (fgetc(cfg_file)!=EOF) s++;
		char buffer[s+1];       //Build a buffer that has the exact size
		
		fseek(cfg_file,0,SEEK_SET);
		
		for (i=0;i<s;i++)
			buffer[i] = fgetc(cfg_file); //Now let's fill it
			
		buffer[i+1] = '\0';
		BString cfg_str = buffer; //We parse the buffer which is a char
								  //and we 'cut' the fields in this BString
								  //with the method CopyInto() 
		fclose(cfg_file);	
			
		int field_begin = 0, field_length;
		BString GLOBAL[14];		  //The GLOBAL: part has 14 fields we'll copy here
		i = 0; 
		
		for (int j=0;j<14;j++){
			field_length = 0 ;
			while (buffer[i] != '=') 
				i++;
			field_begin = i + 2; //Each field begins 2 characters after a '='
				
			while (buffer[i] != '\n') //Each field ends just before a '\n'
				i++;		
			field_length = i-field_begin; 
			
			cfg_str.CopyInto(GLOBAL[j],field_begin,field_length);	
		}	
	
//Now let's see how many interfaces there are and note 
//where each interface's description begins in the file 
		int number_of_interfaces = 0;
		bool first = true;
	
		for (i=0;i<strlen(buffer);i++){
			if (buffer[i]==':'){   //We recognize an interface
				if (first == true) //with the ':' of the "interfacexx :".        
					first = false; //bool first is there to make         
				else {			   //'jump' the ':' of "GLOBAL :"		    					  			
					number_of_interfaces++;
				}
			}										  
		}
	
		int starts[number_of_interfaces];
		first = true;		
		int j =0;								   		
		for (i=0;i<strlen(buffer);i++){
			if (buffer[i]==':'){   
				if (first == true)         
					first = false;          
				else {			   		   
					starts[j] = i;
					j++;  					  			
				}
			}										  
		}		
		//Now let's get the fields of each interface	
		field_begin = 0, field_length;
		BString INTERFACES[number_of_interfaces][8];	//Each interface has 8 fields
	
		for (int k=0;k<number_of_interfaces;k++){
			i = starts[k];	
			for (int j=0;j<8;j++){
				field_length = 0 ;
				while (buffer[i] != '=') 
					i++;
				field_begin = i + 2; //Each field begins 2 characters after a '='
		
				while (buffer[i] != '\n') //Each field ends just before a '\n'
					i++;		
				field_length = i-field_begin; 
		
				cfg_str.CopyInto(INTERFACES[k][j],field_begin,field_length);	
			}
		}																		   
		
//Now let's fill up every field everywhere in the app
		Lock();
	
		fDomain		   ->SetText(GLOBAL[0].String()); //The BTextControls
		fHost  		   ->SetText(GLOBAL[1].String());
		fPrimaryDNS   ->SetText(GLOBAL[9].String());
		fSecondaryDNS ->SetText(GLOBAL[10].String());
	
		if (GLOBAL[5]== "1") fFTPServer->SetValue(1);    //Now the BCheckBoxes
		if (GLOBAL[6]== "1") fIPForwarding->SetValue(1); 
		if (GLOBAL[8]== "1") fTelnetServer->SetValue(1);
		
		for (i=0;i<(unsigned)number_of_interfaces;i++){ //Now the interfaces list
			BString string = INTERFACES[i][3];
			string<<"->"<<INTERFACES[i][5]<<"->"<<"State(Not implemented yet)";
			NetListItem *item = new NetListItem(string.String());
		
			item->fData.fIPAddress  = INTERFACES[i][3];
			item->fData.fNetMask  = INTERFACES[i][4];
			item->fData.fPrettyName = INTERFACES[i][5];
			item->fData.fEnabled  = atoi(INTERFACES[i][6].String());   		
			item->fData.fDHCP	  = atoi(INTERFACES[i][7].String());
		
			fInterfacesList->AddItem(item);
		}	
		fInterfacesList->Select(0L);
	
		Unlock(); 

//And save some of them
		fusername = GLOBAL[2];
	}
*/
}
