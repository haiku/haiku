#include <Alert.h>
#include <image.h>
#include <Font.h>
#include <Messenger.h>
#include <OS.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <stdio.h>
#include <stdlib.h>

#include "Network.h"

#define NETWORK_WINDOW_RIGHT 435
#define NETWORK_WINDOW_BOTTOM 309

#define B_GREY 216,216,216

int main()
{
	Network app;
	app.Run();
	return B_NO_ERROR;
}

Network::Network() : BApplication("application/x-vnd.OBOS-Network")
{
	fwindow = new NetworkWindow();
	fwindow ->Show();
	
	fwindow ->Get_Configs_List();
	fwindow ->GetEverything();
}

NetworkWindow::NetworkWindow() : BWindow(BRect(0, 0, NETWORK_WINDOW_RIGHT, NETWORK_WINDOW_BOTTOM),"Network",B_TITLED_WINDOW,B_NOT_ANCHORED_ON_ACTIVATE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE,B_CURRENT_WORKSPACE) 
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
		fview    = new BView(bounds,"View",B_FOLLOW_ALL_SIDES,B_WILL_DRAW);
	
			frestart = new BButton(BRect(11,274,122,298),"Restart_Networking","Restart Networking",new BMessage(kRestart_Networking_M));
			frevert  = new BButton(BRect(145,274,220,298),"Revert","Revert",new BMessage(kRevert_M));
			fsave	 = new BButton(BRect(349,274,424,297),"Save","Save",new BMessage(kSave_M));
			fsave->MakeDefault(true);
			fsave->SetEnabled(false);
			frevert->SetEnabled(false);
	
			bounds.bottom = Bounds().Height();
			bounds.top += 10;
			bounds.right = Bounds().Width();
			ftabview = new BTabView(bounds,"tab_view", B_WIDTH_FROM_WIDEST);
	
				fidentity       = new BTab();
				bounds.bottom -= ftabview->TabHeight();
				fidentity_view  = new BView(bounds,"Identity",B_FOLLOW_ALL_SIDES,B_WILL_DRAW);
				fservices_view  = new BView(bounds,"Services",B_FOLLOW_ALL_SIDES,B_WILL_DRAW);
					
					bounds.top -= 10;
					bounds.bottom -= 209;
					bounds.left += 11;
					bounds.right  -= 12; 
					fnames 	   	   = new BBox(bounds,"Names",B_FOLLOW_LEFT | B_FOLLOW_TOP,B_WILL_DRAW | B_FRAME_EVENTS);
					
						fdomain     	= new BTextControl(BRect(15,19,195,50),"Domain_Name","Domain name:",NULL,new BMessage(kSOMETHING_HAS_CHANGED_M));  
						fhost			= new BTextControl(BRect(15,49,195,90),"Host_Name","Host name:",NULL,new BMessage(kSOMETHING_HAS_CHANGED_M));   
						fprimary_dns	= new BTextControl(BRect(209,19,399,45),"Primary_DNS","Primary DNS:",NULL,new BMessage(kSOMETHING_HAS_CHANGED_M));  
						fsecondary_dns	= new BTextControl(BRect(209,49,399,90),"Secondary_DNS","Secondary DNS:",NULL,new BMessage(kSOMETHING_HAS_CHANGED_M));   

					bounds.top     = bounds.bottom + 12;
					bounds.bottom  = 208; 
					finterfaces     = new BBox(bounds,"Network_Interfaces",B_FOLLOW_LEFT | B_FOLLOW_TOP,B_WILL_DRAW | B_FRAME_EVENTS); 
					
						fsettings	= new BButton(BRect(325,16,400,37),"Settings","Settings...",new BMessage(kSettings_M));
						fclear		= new BButton(BRect(325,45,400,67),"Clear","Clear",new BMessage(kClear_M));
						fcustom		= new BButton(BRect(325,83,400,107),"Custom","Custom...",new BMessage(kCustom_M));
						//fsettings	->SetEnabled(false);
						//fclear		->SetEnabled(false);
						
						bounds							 = finterfaces->Bounds();
						bounds.InsetBy(20,20);
						bounds.top						 ++;
						bounds.bottom					 += 6;
						bounds.right					 -= 94;
						bounds.left						 -= 5;  
						finterfaces_list 			 = new NetListView(bounds,"Interfaces_List");	
						BScrollView *interfaces_scroller = new BScrollView("Interfaces_Scroller",finterfaces_list, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, 0, 1, B_FANCY_BORDER);	
				
				fservices       = new BTab();
				fftp_server	   = new BCheckBox(BRect(10,20,120,40),"FTP_Server","FTP Server",new BMessage(kSOMETHING_HAS_CHANGED_M));
				ftelnet_server  = new BCheckBox(BRect(10,50,120,70),"Telnet_Server","Telnet Server",new BMessage(kSOMETHING_HAS_CHANGED_M));
				fapple_talk	   = new BCheckBox(BRect(10,110,120,130),"Apple_Talk","Apple Talk",new BMessage(kSOMETHING_HAS_CHANGED_M));
				fip_forwarding  = new BCheckBox(BRect(10,140,120,160),"IP_Forwarding","IP Forwarding",new BMessage(kSOMETHING_HAS_CHANGED_M));
				flogin_info	   = new BButton(BRect(10,80,120,100),"Login_Info","Login Info...",new BMessage(kLogin_Info_M));

				fconfigurations = new BBox(BRect(200,10,460,210),"Configurations",B_FOLLOW_LEFT | B_FOLLOW_TOP,B_WILL_DRAW | B_FRAME_EVENTS); 							
						
						bounds = fconfigurations->Bounds();
						bounds.InsetBy(20,20);
						bounds.right -= 80;
						fconfigurations_list 			 = new NetListView(bounds,"Configurations_List");	
						BScrollView *configurations_scroller = new BScrollView("Configurations_Scroller",fconfigurations_list);	
												
						fbackup   = new BButton(BRect(170,17,250,37),"Backup","Backup",new BMessage(kBackup_M));
						frestore  = new BButton(BRect(170,47,250,67),"Restore","Restore",new BMessage(kRestore_M));
						fremove   = new BButton(BRect(170,87,250,107),"Delete","Delete",new BMessage(kDelete_M));
						frestore->SetEnabled(false);
						fremove->SetEnabled(false);
						
				BBox *hr = new BBox(BRect(1,261,Bounds().Width()-1,262), "horizontal_rule");
				BBox *vr = new BBox(BRect(133, 274, 134, 299), "vertical_rule");
				
	fdomain->SetDivider(70);
	fdomain->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	fhost->SetDivider(70);
	fhost->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	fprimary_dns->SetDivider(80);
	fprimary_dns->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	fsecondary_dns->SetDivider(80);
	fsecondary_dns->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);			
	
	fnames			->SetLabel("Names");
	finterfaces		->SetLabel("Network Interfaces");
	fconfigurations	->SetLabel("Configurations");
	fidentity_view	->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fservices_view	->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	ftabview		->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));	
	fview			->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
//Casts an AddEverything 
	ftabview			->AddTab(fidentity_view,fidentity);
	ftabview			->AddTab(fservices_view,fservices);
		
	fnames			->AddChild(fdomain);
	fnames			->AddChild(fhost);
	fnames			->AddChild(fprimary_dns);
	fnames			->AddChild(fsecondary_dns);
	finterfaces		->AddChild(interfaces_scroller);
	finterfaces		->AddChild(fsettings);
	finterfaces		->AddChild(fclear);
	finterfaces		->AddChild(fcustom);					
	fidentity_view	->AddChild(fnames);
	fidentity_view	->AddChild(finterfaces);
	fservices_view	->AddChild(fftp_server);
	fservices_view	->AddChild(ftelnet_server);
	fservices_view	->AddChild(flogin_info);
	fservices_view	->AddChild(fapple_talk);
	fservices_view	->AddChild(fip_forwarding);
	fservices_view	->AddChild(fconfigurations);
	fconfigurations	->AddChild(configurations_scroller);
	fconfigurations	->AddChild(fbackup);
	fconfigurations	->AddChild(frestore);
	fconfigurations	->AddChild(fremove);							
	fview			->AddChild(ftabview);
	fview			->AddChild(frestart);
	fview			->AddChild(frevert);	
	fview			->AddChild(fsave);
	fview			->AddChild(hr);
	fview			->AddChild(vr);
	AddChild(fview);	
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
			frevert->SetEnabled(false);
			fsave->SetEnabled(false);			
			break;
		}
		case kSave_M:{
			BackupWin *bak = new BackupWin();
			bak->fparent_window = dynamic_cast <NetworkWindow *> (fview->Window());
			bak->Flatten_bak();
			bak->Quit();
			frevert->SetEnabled(false);
			fsave->SetEnabled(false);			
			break;
		}
		case kSettings_M:{
			InterfaceWin *win = new InterfaceWin();
			
			win->fparent_window = dynamic_cast <NetworkWindow *> (fview->Window());
			
			NetListItem *selection = dynamic_cast <NetListItem *> (finterfaces_list->ItemAt(finterfaces_list->CurrentSelection()));
			
			if (selection->fENABLED == 1)
				win->finter_enabled->SetValue(1);
			if (selection->fDHCP == 1)
				win->finter_dhcp->SetValue(1);
			else
				win->finter_manual->SetValue(1);		
			
			win->finter_name->SetText((selection->fPRETTYNAME).String());		
			win->finter_ip->SetText((selection->fIPADDRESS).String());
			win->finter_mask->SetText((selection->fNETMASK).String());
			
			win->fhas_something_changed = false;
			
			win->finter_ip->SetTarget(win);
			win->finter_mask->SetTarget(win);
			win->finter_gateway->SetTarget(win); 
						
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
			login_info_pop->fparent_window = dynamic_cast <NetworkWindow *> (fview->Window());
			login_info_pop->flogin_info_name->SetText(fusername.String());
			login_info_pop->flogin_info_name->MakeFocus();
			login_info_pop->Show();
			break;
		}
		case kBackup_M:{
			BackupWin *backup_pop = new BackupWin();
			backup_pop->fparent_window = dynamic_cast <NetworkWindow *> (fview->Window());
			backup_pop->fbak_name->MakeFocus();	
			backup_pop->Show();
			break;
		}
		case kRestore_M:{
			BStringItem *string_item = dynamic_cast <BStringItem *> (fconfigurations_list->ItemAt(fconfigurations_list->CurrentSelection()));
			GetEverything(string_item->Text());
			PostMessage(kSOMETHING_HAS_CHANGED_M);
			break;
		}
		case kDelete_M:{
			Remove_Config_File();
			Get_Configs_List();
			fconfigurations_list->SelectionChanged();
			break;
		}	
		case kSOMETHING_HAS_CHANGED_M:{
			frevert->SetEnabled(true);
			fsave->SetEnabled(true);	
			break;	
		}
	}
}

bool NetworkWindow::QuitRequested()
{
	if (fsave->IsEnabled() == true){
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
				BackupWin *sav = new BackupWin();
				sav->fparent_window = dynamic_cast <NetworkWindow *> (fview->Window());
				sav->Flatten_bak();
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

LoginInfo::LoginInfo() : BWindow(BRect(410,200,710,335),"Login Info",B_MODAL_WINDOW,B_NOT_ANCHORED_ON_ACTIVATE | B_NOT_RESIZABLE,B_CURRENT_WORKSPACE)
{
	flogin_info_view 	  = new BView(Bounds(),"Login_Info_View",B_FOLLOW_ALL_SIDES,B_WILL_DRAW); 

	flogin_info_name 	  = new BTextControl(BRect(10,10,290,30),"User_Name","User Name",NULL,new BMessage());  
	flogin_info_password  = new BTextControl(BRect(10,40,290,60),"Password","Password",NULL,new BMessage());  
	flogin_info_confirm   = new BTextControl(BRect(10,70,290,90),"Confirm Password","Confirm Password",NULL,new BMessage());  
	flogin_info_cancel	  = new BButton(BRect(100,100,190,120),"Cancel","Cancel",new BMessage(kLogin_Info_Cancel_M));
	flogin_info_done	  = new BButton(BRect(200,100,290,120),"Done","Done",new BMessage(kLogin_Info_Done_M));
	
	flogin_info_done->MakeDefault(true);
	flogin_info_view->SetViewColor(B_GREY);
	flogin_info_view->AddChild(flogin_info_name);
	flogin_info_view->AddChild(flogin_info_password);
	flogin_info_view->AddChild(flogin_info_confirm);
	flogin_info_view->AddChild(flogin_info_cancel);
	flogin_info_view->AddChild(flogin_info_done);		
	
	AddChild(flogin_info_view);
}

void LoginInfo::MessageReceived(BMessage *message)
{
	switch (message->what)
		{
		case kLogin_Info_Cancel_M:
			{
			Quit();
			break;
			}
		case kLogin_Info_Done_M:
			{		
			const char *login_info_name_S 	  = flogin_info_name	  ->Text();
			const char *login_info_password_S = flogin_info_password  ->Text();
			const char *login_info_confirm_S  = flogin_info_confirm   ->Text();
			
			if (strlen(login_info_name_S)        > 0 
				&& strlen(login_info_password_S) > 0 
				&& strlen(login_info_confirm_S)  > 0 ){
					if (strcmp(login_info_password_S,login_info_confirm_S) == 0){
						fparent_window->flogin_info_S[0] = login_info_name_S;
						fparent_window->flogin_info_S[1] = login_info_password_S;											
						fparent_window->PostMessage(fparent_window->kSOMETHING_HAS_CHANGED_M);
						Quit();
					}
					else{
						BAlert *alert = new BAlert("Login Info Alert","Youd didn't enter twice the same password","Oups...",NULL,NULL,B_WIDTH_FROM_WIDEST,B_INFO_ALERT);
						alert->Go();
					}	
							
				}
			else{
				BAlert *alert = new BAlert("Login Info Alert","You didn't fill all the fields","Oups...",NULL,NULL,B_WIDTH_FROM_WIDEST,B_INFO_ALERT);
				alert->Go();
			}	
			break;
			}
		}			
}

BackupWin::BackupWin() : BWindow(BRect(410,200,710,285),"Backup Win",B_MODAL_WINDOW,B_NOT_ANCHORED_ON_ACTIVATE | B_NOT_RESIZABLE,B_CURRENT_WORKSPACE)
{
	fbakview 	= new BView(Bounds(),"Backup",B_FOLLOW_ALL_SIDES,B_WILL_DRAW); 
	fbak_name	= new BTextControl(BRect(10,20,290,40),"Backup As","Backup As:",NULL,new BMessage());  
	fbak_cancel	= new BButton(BRect(100,50,190,70),"Cancel","Cancel",new BMessage(kCancel_bak_M));
	fbak_done   = new BButton(BRect(200,50,290,70),"Done","Done",new BMessage(kFlatten_bak_M));
	
	fbakview->SetViewColor(B_GREY);
	fbak_done->MakeDefault(true);
	
	fbakview->AddChild(fbak_name);
	fbakview->AddChild(fbak_cancel);
	fbakview->AddChild(fbak_done);

	AddChild(fbakview);	
}
	
void BackupWin::MessageReceived(BMessage *message)
{
	switch (message->what){
		case kCancel_bak_M:{
			Quit();
			break;
		}
		case kFlatten_bak_M:{		
			const char *bak_name_S = fbak_name->Text();
			
			if (strlen(bak_name_S) > 0){			
				Flatten_bak(bak_name_S);     //Writes the new config file
				fparent_window->Lock();
				fparent_window->Get_Configs_List(); //Update the list
				fparent_window->fconfigurations_list->SelectionChanged(); //Update the buttons
				fparent_window->Unlock();							
				Quit();
			}
			else{
				BAlert *alert = new BAlert("Backup Info Alert","You must sprcify a name","Oups...",NULL,NULL,B_WIDTH_FROM_WIDEST,B_INFO_ALERT);
				alert->Go();
			}	
			break;
		}
	}			
}
	
NetListView::NetListView(BRect frame,const char *name) : BListView(frame, name)
{} //The NetListView only exists to override the SelectionChanged() function below

//Buttons are enabled or not depending on the selection in the list
//so each time you click in the list, this method is called to update the buttons
void NetListView::SelectionChanged(void)
{
	int32 index = CurrentSelection();

	BView *parent = Parent();
	
	if (index >= 0){			
		if (strcmp(parent->Name(),"Interfaces_Scroller") == 0) //There are 2 lists that are instances of this class, 
			{												  //so we have to see in which list you are clicking
			BButton *settings = dynamic_cast <BButton *> ((parent->Parent())->FindView("Settings"));
			BButton *clear 	  = dynamic_cast <BButton *> ((parent->Parent())->FindView("Clear"));
			
			settings ->SetEnabled(true);
			clear    ->SetEnabled(true);
		}
		else if (strcmp(parent->Name(),"Configurations_Scroller") == 0){				
			BButton *restore = dynamic_cast <BButton *> ((parent->Parent())->FindView("Restore"));
			BButton *remove	 = dynamic_cast <BButton *> ((parent->Parent())->FindView("Delete"));
			
			restore ->SetEnabled(true);
			remove  ->SetEnabled(true);
		}
	}
	else{
		if (strcmp(parent->Name(),"Interfaces_Scroller") == 0) //There are 2 lists that are instances of this class, 
			{												  //so we have to see in which list you are clicking
			BButton *settings = dynamic_cast <BButton *> ((parent->Parent())->FindView("Settings"));
			BButton *clear 	  = dynamic_cast <BButton *> ((parent->Parent())->FindView("Clear"));
			
			settings ->SetEnabled(false);
			clear    ->SetEnabled(false);
		}
		else if (strcmp(parent->Name(),"Configurations_Scroller") == 0){				
			BButton *restore = dynamic_cast <BButton *> ((parent->Parent())->FindView("Restore"));
			BButton *remove	 = dynamic_cast <BButton *> ((parent->Parent())->FindView("Delete"));
			
			restore ->SetEnabled(false);
			remove  ->SetEnabled(false);
		}
	}			
				
}

RestartWindow::RestartWindow() : BWindow(BRect(400,300,650,350),"Restart Networking",B_MODAL_WINDOW,B_NOT_ANCHORED_ON_ACTIVATE | B_NOT_RESIZABLE,B_CURRENT_WORKSPACE)
{
	fview = new BView(Bounds(),"Restart_View",B_FOLLOW_ALL_SIDES,B_WILL_DRAW);
	
	fview->SetViewColor(B_GREY);
	
	frestarting     = new BStringView(BRect(60,5,200,25),"Restarting_Text_1","Restarting Networking");
	fwhatshappening = new BStringView(BRect(60,30,200,50),"Restarting_Text_2","Please Wait..."); 
	
	BFont font(be_bold_font);
	
	frestarting->SetFont(&font);
	fwhatshappening->SetFont(&font);
	
	fview->AddChild(frestarting);
	fview->AddChild(fwhatshappening);

	AddChild(fview);		
}

NetListItem::NetListItem(const char *text) : BStringItem(text)
{}
	 
InterfaceWin::InterfaceWin():BWindow(BRect(410,350,720,600),"Interface Win",B_MODAL_WINDOW,B_NOT_ANCHORED_ON_ACTIVATE | B_NOT_RESIZABLE,B_CURRENT_WORKSPACE)
{
	BRect rect = Bounds();
	finterview 	  = new BView(rect,"Interface_View",B_FOLLOW_ALL_SIDES,B_WILL_DRAW); 
	finter_name	  = new BStringView(BRect(10,10,290,20),"Interface_Name",NULL);
	finter_enabled= new BCheckBox(BRect(10,30,190,40),"Enabled","Interface enabled",new BMessage(kSOMETHING_HAS_CHANGED));
	finter_dhcp   = new BRadioButton(BRect(10,50,290,60),"DHCP","Obtain settings automatically (DHCP)",new BMessage(kSOMETHING_HAS_CHANGED));	
	finter_manual = new BRadioButton(BRect(10,70,290,80),"Manual","Specify settings",new BMessage(kSOMETHING_HAS_CHANGED));
	finter_config = new BButton(BRect(10,220,100,230),"Config","Config...",new BMessage());
	finter_cancel = new BButton(BRect(145,220,215,230),"Cancel","Cancel",new BMessage(kCancel_inter_M));
	finter_done   = new BButton(BRect(225,220,295,230),"Done","Done",new BMessage(kDone_inter_M));
	rect.InsetBy(5,5);
	rect.top+=95;
	rect.bottom-=35;
	finter_box	  = new BBox(rect,"IP",B_FOLLOW_LEFT | B_FOLLOW_TOP,B_WILL_DRAW | B_FRAME_EVENTS);
		finter_ip	   = new BTextControl(BRect(10,15,290,35),"IP address","IP address:",NULL,new BMessage(kSOMETHING_HAS_CHANGED));  
		finter_mask	   = new BTextControl(BRect(10,45,290,65),"Subnet mask","Subnet mask:",NULL,new BMessage(kSOMETHING_HAS_CHANGED)); 
		finter_gateway = new BTextControl(BRect(10,75,290,95),"Gateway","Gateway:",NULL,new BMessage(kSOMETHING_HAS_CHANGED)); 
		
	finterview->SetViewColor(B_GREY);
	finter_done->MakeDefault(true);
	
	finter_box->AddChild(finter_ip);
	finter_box->AddChild(finter_mask);
	finter_box->AddChild(finter_gateway);
	
	finterview->AddChild(finter_name);
	finterview->AddChild(finter_enabled);
	finterview->AddChild(finter_dhcp);
	finterview->AddChild(finter_manual);	
	finterview->AddChild(finter_config);
	finterview->AddChild(finter_cancel);
	finterview->AddChild(finter_done);
	finterview->AddChild(finter_box);
	
	AddChild(finterview);
}	

void InterfaceWin::MessageReceived(BMessage *message)
{
	switch(message->what){
		case kCancel_inter_M:{
			Quit();
			break;
		}
		case kDone_inter_M:{
			if (fhas_something_changed == true){
				fparent_window->PostMessage(fparent_window->kSOMETHING_HAS_CHANGED_M);
				
				NetListItem *item = dynamic_cast <NetListItem *> (fparent_window->finterfaces_list->ItemAt(fparent_window->finterfaces_list->CurrentSelection()));
				
				item->fIPADDRESS = finter_ip->Text();
				item->fNETMASK 	 = finter_mask->Text();
				
				if (finter_enabled->Value() == 0)
					item->fENABLED	= 0;
				else
					item->fENABLED	= 1;
				
				if (finter_dhcp->Value() == 0)
					item->fDHCP = 0;
				else
					item->fDHCP = 1;			 
			}	
			Quit();
			break;
		}
		case kSOMETHING_HAS_CHANGED:{
			fhas_something_changed = true;
			break;
		}
	}			
}


