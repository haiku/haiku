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
#include <Region.h>
#include <Path.h>

#include "InterfaceWin.h"
#include "BackupWindow.h"
#include "LoginInfo.h"

//#define NETWORK_WINDOW_RIGHT 435
//#define NETWORK_WINDOW_BOTTOM 309

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
	
	fRestarting = new BStringView(BRect(60,5,200,25),"Restarting_Text_1",
									"Restarting Networking");
	fWhatsHappening = new BStringView(BRect(60,30,200,50),"Restarting_Text_2",
									"Please Wait..."); 
	
	BFont font(be_bold_font);
	
	fRestarting->SetFont(&font);
	fWhatsHappening->SetFont(&font);
	
	fView->AddChild(fRestarting);
	fView->AddChild(fWhatsHappening);

	AddChild(fView);
}

NetworkWindow::NetworkWindow()
  : BWindow(BRect(0, 0, 435, 309),"Network",
			B_TITLED_WINDOW,B_NOT_ANCHORED_ON_ACTIVATE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE,
			B_CURRENT_WORKSPACE) 
{
	BScreen screen;
	
	BRect screenframe = screen.Frame(), bounds = Bounds(), workrect(0,0,1,1);
	float width,height;
	float screenx, screeny;
	
	screenx = ((screenframe.right + 1) / 2) - (bounds.right / 2) - 1;
	screeny = ((screenframe.bottom + 1) / 2) - (bounds.bottom / 2) - 1;
	MoveTo(screenx, screeny);
	
	fView = new BView(bounds,"View",B_FOLLOW_ALL_SIDES,B_WILL_DRAW);
	fView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fView);
	
	fRestart = new BButton(workrect,"Restart_Networking",
							"Restart Networking",new BMessage(kRestart_Networking_M),
							B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fRestart->GetPreferredSize(&width,&height);
	fRestart->ResizeTo(width,height);
	fRestart->MoveTo(10,bounds.bottom - 10 - height);
	
	// We fake the divider line by using a BBox. We have to call ConstrainClippingRegion
	// because otherwise we also see the corners drawn, which doesn't look nice
	BBox *vr = new BBox(BRect(width + 18, bounds.bottom - 11 - height, width + 19,
								bounds.bottom - 9),
						"vertical_rule", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fView->AddChild(vr);
	
	BRegion clipreg(vr->Bounds().InsetByCopy(0,2));
	vr->ConstrainClippingRegion(&clipreg);
	
	fRevert = new BButton(BRect(width + 25,bounds.bottom - 10 - height,width + 26,
								bounds.bottom - 10),
						"Revert","Revert", new BMessage(kRevert_M),
						B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fRevert->ResizeToPreferred();
	fRevert->SetEnabled(false);
	
	fSave = new BButton(workrect,"Save","Save",new BMessage(kSave_M),
						B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fSave->GetPreferredSize(&width,&height);
	fSave->ResizeTo(width,height);
	fSave->MoveTo(bounds.right - 10 - width, bounds.bottom - 10 - height);
	fSave->MakeDefault(true);
	fSave->SetEnabled(false);
	
	BBox *hr = new BBox(BRect(0,bounds.bottom - height - 20,bounds.right,
								bounds.bottom - height - 19),
						"horizontal_rule",B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	fView->AddChild(hr);
	
	workrect = Bounds();
	workrect.bottom = hr->Frame().top;
	workrect.top = 10;
	
	fTabView = new BTabView(workrect,"tab_view",B_WIDTH_FROM_WIDEST, B_FOLLOW_ALL);
	fTabView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));	
	fView->AddChild(fTabView);
	bounds.bottom -= fTabView->TabHeight();

	fView->AddChild(fRestart);
	fView->AddChild(fRevert);	
	fView->AddChild(fSave);
	
	bounds = fTabView->Bounds();
	bounds.bottom -= fTabView->TabHeight();
	
	fIdentityView = new BView(bounds,"Identity",B_FOLLOW_ALL_SIDES,B_WILL_DRAW);
	fIdentityView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fIdentity = new BTab();
	fTabView->AddTab(fIdentityView,fIdentity);
	
	// We can just pick a view to call StringWidth because we're not using anything
	// except the unaltered be_plain_font. Note that for each BTextControl pair we
	// just use the longer word of the two in order to save us some speed and code size
	float labelwidth = fTabView->StringWidth("Domain name: ");
	
	workrect.Set(10,20,11,21);
	fDomain = new BTextControl(workrect,"Domain_Name","Domain name:"," ",
							new BMessage(kSOMETHING_HAS_CHANGED_M));  
	
	// BTextControl::ResizeToPreferred() is a little strange for our purposes, so we
	// get the preferred size and ignore the width that we get.
	fDomain->GetPreferredSize(&width, &height);
	fDomain->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	fDomain->SetDivider(labelwidth);
	
	workrect.left = 10;
	workrect.right = 11;
	workrect.top = height + 30;
	workrect.bottom = workrect.top + height;
	
	fHost = new BTextControl(workrect,"Host_Name","Host name:",NULL,
							new BMessage(kSOMETHING_HAS_CHANGED_M));   
	fHost->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	fHost->SetDivider(labelwidth);
	
	labelwidth = fTabView->StringWidth("Secondary DNS: ");
	
	workrect.OffsetTo((bounds.right / 2),20);
	workrect.right = bounds.right - 30;
	fPrimaryDNS = new BTextControl(workrect,"Primary_DNS","Primary DNS:",
								NULL,new BMessage(kSOMETHING_HAS_CHANGED_M),
								B_FOLLOW_TOP | B_FOLLOW_RIGHT);  
	fPrimaryDNS->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	fPrimaryDNS->SetDivider(labelwidth);
	
	workrect.OffsetBy(0,height + 10);
	fSecondaryDNS = new BTextControl(workrect,"Secondary_DNS","Secondary DNS:",
								NULL,new BMessage(kSOMETHING_HAS_CHANGED_M),
								B_FOLLOW_TOP | B_FOLLOW_RIGHT);  
	fSecondaryDNS->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);			
	fSecondaryDNS->SetDivider(labelwidth);
	
	fNames = new BBox(BRect(10,10,bounds.right - 10, (height * 2) + 50),
					"Names",B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,B_WILL_DRAW | 
					B_FRAME_EVENTS);
	fIdentityView->AddChild(fNames);
	fNames->SetLabel("Names");
	
	fNames->AddChild(fDomain);
	fNames->AddChild(fHost);
	fNames->AddChild(fPrimaryDNS);
	fNames->AddChild(fSecondaryDNS);
	
	// BTextControl resizing operations don't do anything under R5 unless they
	// have an owner. To make things worse, BTabView does some of its BView hierarchy
	// stuff in AttachedToWindow such that if you add a tab before it's attached to 
	// a window, it won't be visible. :( As such, we make an extra call to 
	// AttachedToWindow to be assured that everything works the way it is intended.
	fTabView->AttachedToWindow();
	fDomain->ResizeTo( (bounds.right / 2) - 25, height);
	fHost->ResizeTo( (bounds.right / 2) - 25, height);
	
	workrect.left = 10;
	workrect.right = bounds.right - 10;
	workrect.top = fNames->Frame().bottom + 10;
	workrect.bottom = fIdentityView->Bounds().bottom - 15;
	fInterfaces = new BBox(workrect,"Network_Interfaces",B_FOLLOW_ALL,
						B_WILL_DRAW | B_FRAME_EVENTS); 
	fInterfaces->SetLabel("Network Interfaces");
	fIdentityView->AddChild(fInterfaces);
	
	workrect = fInterfaces->Bounds().InsetByCopy(5,5);
	fSettings = new BButton(BRect(0,0,1,1),"Settings","Settings" B_UTF8_ELLIPSIS,
							new BMessage(kSettings_M),
							B_FOLLOW_TOP | B_FOLLOW_RIGHT);
	fSettings->ResizeToPreferred();
	fSettings->MoveTo(workrect.right - 5 - fSettings->Bounds().Width(), 15);
	fInterfaces->AddChild(fSettings);
	
	workrect = fSettings->Frame();
	workrect.OffsetBy(0,workrect.Height()+5);
	
	fClear = new BButton(workrect,"Clear","Clear",new BMessage(kClear_M),
						B_FOLLOW_TOP | B_FOLLOW_RIGHT);
	fInterfaces->AddChild(fClear);
	
	workrect.OffsetBy(0,workrect.Height()+15);
	fCustom = new BButton(workrect,"Custom","Custom" B_UTF8_ELLIPSIS,new BMessage(kCustom_M),
							B_FOLLOW_TOP | B_FOLLOW_RIGHT);
	fInterfaces->AddChild(fCustom);
	
	if(workrect.bottom > fInterfaces->Bounds().bottom)
		ResizeBy(0,workrect.bottom - fInterfaces->Bounds().bottom + 10);
	
	workrect.left = 10;
	workrect.top = 20;
	workrect.right = fCustom->Frame().left - 15 - B_V_SCROLL_BAR_WIDTH;
	workrect.bottom = fCustom->Frame().bottom - 5;
	
	fInterfacesList = new NetListView(workrect,"Interfaces_List");	
	BScrollView *interfaces_scroller = new BScrollView("Interfaces_Scroller",
														fInterfacesList, B_FOLLOW_ALL,
														0, 0, 1, B_FANCY_BORDER);	
	fInterfaces->AddChild(interfaces_scroller);
	
	fServicesView = new BView(bounds,"Services",B_FOLLOW_ALL_SIDES,B_WILL_DRAW);
	fServicesView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fServices = new BTab();
	fTabView->AddTab(fServicesView,fServices);
	
	workrect.Set(10,10,11,11);
	fFTPServer = new BCheckBox(workrect,"FTP_Server","FTP Server",
								new BMessage(kSOMETHING_HAS_CHANGED_M));
	fFTPServer->GetPreferredSize(&width,&height);
	fFTPServer->ResizeTo(width,height);
	fServicesView->AddChild(fFTPServer);
	
	fTelnetServer = new BCheckBox(workrect,"Telnet_Server","Telnet Server",
								new BMessage(kSOMETHING_HAS_CHANGED_M));
	fTelnetServer->ResizeToPreferred();
	fTelnetServer->MoveTo(10,height + 15);
	fServicesView->AddChild(fTelnetServer);
	
	fLoginInfo = new BButton(workrect,"Login_Info","Login Info" B_UTF8_ELLIPSIS,
								new BMessage(kLogin_Info_M));
	fLoginInfo->ResizeToPreferred();
	fLoginInfo->MoveTo(10,(height * 2) + 25);
	fServicesView->AddChild(fLoginInfo);
	
	workrect = fLoginInfo->Frame();
	workrect.left -= 5;
	workrect.top = workrect.bottom + 10;
	workrect.bottom = workrect.top + 1;
	workrect.right += 70;
	BBox *hr2 = new BBox(workrect,"horizontal_rule2",B_FOLLOW_LEFT | B_FOLLOW_TOP);
	fServicesView->AddChild(hr2);
	
	
	fAppleTalk = new BCheckBox(workrect,"Apple_Talk","Apple Talk",
								new BMessage(kSOMETHING_HAS_CHANGED_M));
	fAppleTalk->ResizeToPreferred();
	fAppleTalk->MoveTo(10, workrect.bottom + 10);
	fServicesView->AddChild(fAppleTalk);
	
	fIPForwarding = new BCheckBox(workrect,"IP_Forwarding","IP Forwarding",
								new BMessage(kSOMETHING_HAS_CHANGED_M));
	fIPForwarding->ResizeToPreferred();
	fIPForwarding->MoveTo(10, fAppleTalk->Frame().bottom + 5);
	fServicesView->AddChild(fIPForwarding);
	
	workrect = bounds.InsetByCopy(10,10);
	workrect.bottom += 10;
	workrect.left = hr2->Frame().right + 10;
	fConfigurations = new BBox(workrect,"Configurations",B_FOLLOW_ALL,
								B_WILL_DRAW | B_FRAME_EVENTS); 							
	fConfigurations->SetLabel("Configurations");
	fServicesView->AddChild(fConfigurations);
	
	workrect = fConfigurations->Bounds();
	fBackup = new BButton(BRect(0,0,1,1),"Backup","Backup", new BMessage(kBackup_M));
	fBackup->GetPreferredSize(&width,&height);
	fBackup->ResizeTo(width,height);
	fBackup->MoveTo(workrect.right - width - 10,15);
	fConfigurations->AddChild(fBackup);
	
	fRestore = new BButton(BRect(0,0,width,height),"Restore","Restore",
							new BMessage(kRestore_M));
	fConfigurations->AddChild(fRestore);
	fRestore->MoveTo(workrect.right - width - 10, 20 + height);
	fRestore->SetEnabled(false);
	
	fRemove = new BButton(BRect(0,0,width,height),"Delete","Delete",
							new BMessage(kDelete_M));
	fRemove->MoveTo(workrect.right - width - 10, 25 + (height * 2));
	fRemove->SetEnabled(false);
	fConfigurations->AddChild(fRemove);

	workrect = fConfigurations->Bounds().InsetByCopy(15,15);
	workrect.top += 5;
	workrect.right = fBackup->Frame().left - 10 - B_V_SCROLL_BAR_WIDTH;
	fConfigurationsList = new NetListView(workrect,"Configurations_List");	
	BScrollView *configurations_scroller = new BScrollView("Configurations_Scroller",
														fConfigurationsList, 
														B_FOLLOW_ALL, 0, 0, 1,
														B_FANCY_BORDER);	
	fConfigurations->AddChild(configurations_scroller);
	
	LoadConfigEntries();
	LoadSettings();
	
	fTabView->AttachedToWindow();
}

void NetworkWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kRestart_Networking_M: {
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
		case kRevert_M: {
			LoadSettings();
			fRevert->SetEnabled(false);
			fSave->SetEnabled(false);			
			break;
		}
		case kSave_M: {
			SaveBackup(NULL,fData);
			fRevert->SetEnabled(false);
			fSave->SetEnabled(false);			
			break;
		}
		case kSettings_M: {
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
		case kClear_M: {
			break;
		}
		case kCustom_M: {
			break;
		}
		case kLogin_Info_M: {
			LoginInfo *login_info_pop = new LoginInfo();
			login_info_pop->fParentWindow = dynamic_cast <NetworkWindow *> (fView->Window());
			login_info_pop->fName->SetText(fusername.String());
			login_info_pop->fName->MakeFocus();
			login_info_pop->Show();
			break;
		}
		case kBackup_M: {
			BRect r(Frame());
			r.right = r.left + 260;
			r.bottom = r.top + 77;
			r.OffsetBy( (Frame().Width()-260)/2, (Frame().Height()-77)/2);
			BackupWin *backup_pop = new BackupWin(r);
			backup_pop->fParentWindow = dynamic_cast <NetworkWindow *> (fView->Window());
			backup_pop->Show();
			break;
		}
		case kRestore_M: {
			BStringItem *string_item = dynamic_cast <BStringItem *> (fConfigurationsList->ItemAt(fConfigurationsList->CurrentSelection()));
			LoadSettings(string_item->Text());
			PostMessage(kSOMETHING_HAS_CHANGED_M);
			break;
		}
		case kDelete_M: {
			DeleteConfigFile();
			LoadConfigEntries();
			fConfigurationsList->SelectionChanged();
			break;
		}	
		case kSOMETHING_HAS_CHANGED_M: {
			fRevert->SetEnabled(true);
			fSave->SetEnabled(true);	
			break;	
		}
	}
}

bool
NetworkWindow::QuitRequested(void)
{
	if (fSave->IsEnabled() == true) {
		BAlert *alert = new BAlert("Save Info Alert", "Save changes before "
			quitting?",	"Don't Save", "Cancel", "Save");
		alert->SetShortcut(1, B_ESCAPE);
		int32 result = alert->Go();
		
		switch (result) {
			case 0: {
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
				SaveBackup(NULL,fData);
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
	BAlert *alert = new BAlert("Alert", "Really delete networking configuration?",
								"Delete", "Cancel");
	alert->SetShortcut(1, B_ESCAPE);
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

void
NetworkWindow::LoadConfigEntries()
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

void
NetworkWindow::LoadSettings(const char *filename)
{
	if(LoadBackup(filename,fData)!=B_OK)
		return;
	
	Lock();
	for(int32 i=0; i<fData.fInterfaceList.CountItems(); i++) {
		InterfaceData *idata = (InterfaceData*)fData.fInterfaceList.ItemAt(i);
		fInterfacesList->AddItem(new NetListItem(*idata));
	}
	fInterfacesList->Select(0L);
	
	fDomain->SetText(fData.fDomainName.String());
	fHost->SetText(fData.fHostname.String());
	fPrimaryDNS->SetText(fData.fPrimaryDNS.String());
	fSecondaryDNS->SetText(fData.fSecondaryDNS.String());

	if(fData.fFTPServer)
		fFTPServer->SetValue(B_CONTROL_ON);
	if(fData.fIPForwarding)
		fIPForwarding->SetValue(B_CONTROL_ON); 
	if(fData.fTelnetServer)
		fTelnetServer->SetValue(B_CONTROL_ON);

	Unlock(); 
}
