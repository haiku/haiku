#include <Alert.h>
#include <stdio.h>
#include <File.h>
#include "BackupWindow.h"
#include "NetworkWindow.h"
#include "NetListView.h"
#include "ConfigData.h"

BackupWin::BackupWin(const BRect &frame)
 :	BWindow(frame,"Backup Win",B_MODAL_WINDOW,
			B_NOT_ANCHORED_ON_ACTIVATE | B_NOT_RESIZABLE,B_CURRENT_WORKSPACE)
{
	BRect r(Bounds());
	
	BView *view=new BView(Bounds(),"Backup",B_FOLLOW_ALL_SIDES,B_WILL_DRAW); 
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);	
	
	fDone=new BButton(BRect(0,0,1,1),"Done","Done",new BMessage(kSaveBackup_M));
	fDone->ResizeToPreferred();
	fDone->MoveTo(r.right - 5 - fDone->Bounds().Width(),
					r.bottom - 5 - fDone->Bounds().Height());
	
	fCancel=new BButton(BRect(100,50,190,70),"Cancel","Cancel",
							new BMessage(kCancel_bak_M));
	fCancel->ResizeToPreferred();
	fCancel->MoveTo(fDone->Frame().left - 5 - fCancel->Frame().Width(),
				fDone->Frame().top);
	
	fName=new BTextControl(BRect(5,5,r.right - 10,40),"Backup As","Backup As:",
								NULL,new BMessage(),B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	float w,h;
	fName->GetPreferredSize(&w,&h);
	fName->ResizeTo(r.right - 10, h);
	fName->MoveTo(5, 5 + (fCancel->Frame().top-h-10)/2);
	fName->SetDivider(fName->StringWidth("Backup As:")+10);
	
	view->AddChild(fName);
	view->AddChild(fCancel);
	view->AddChild(fDone);
	fDone->MakeDefault(true);
	fName->MakeFocus(true);
}
	
void
BackupWin::MessageReceived(BMessage *message)
{
	switch (message->what) {
	
		case kCancel_bak_M: {
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case kSaveBackup_M: {
			
			if (strlen(fName->Text()) > 0) {
				SaveBackup(fName->Text());     //Writes the new config file
				fParentWindow->Lock();
				fParentWindow->LoadConfigEntries(); //Update the list
				fParentWindow->fConfigurationsList->SelectionChanged(); //Update the buttons
				fParentWindow->Unlock();							
				Quit();
			}
			else {
				BAlert *alert = new BAlert("Backup Info Alert","You must specify a name.","OK");
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				alert->Go();
			}	
			break;
		}
	}			
}

void
BackupWin::SaveBackup(const char *bak_name)
{
/*
	// GLOBAL: section
	BString DNS_DOMAIN  	 = "\n\tDNS_DOMAIN = ";
			DNS_DOMAIN       << fParentWindow->fDomain->Text();
	BString HOSTNAME    	 = "\n\tHOSTNAME = ";
			HOSTNAME         << fParentWindow->fHost->Text();
	BString USERNAME  	     = "\n\tUSERNAME = ";
			USERNAME		 << fParentWindow->flogin_info_S[0];
	BString PROTOCOLS 	     = "\n\tPROTOCOLS = ";//where to get that info ??
	BString PASSWORD		 = "\n\tPASSWORD = ";
			PASSWORD         << fParentWindow->flogin_info_S[1];
	BString FTP_ENABLED		 = "\n\tFTP_ENABLED = "; 
		if (fParentWindow->fFTPServer->Value() == 0)
			     FTP_ENABLED << 0; 
		else     FTP_ENABLED << 1;
	BString IP_FORWARD 		 = "\n\tIP_FORWARD = ";
		if (fParentWindow->fIPForwarding->Value() == 0)
			      IP_FORWARD << 0;
		else      IP_FORWARD << 1;
	BString DNS_ENABLED	     = "\n\tDNS_ENABLED = ";//where to get that info ??
	BString TELNETD_ENABLED  = "\n\tTELNETD_ENABLED = ";
		if (fParentWindow->fTelnetServer->Value() == 0)
			 TELNETD_ENABLED << 0;
		else TELNETD_ENABLED << 1;
	BString DNS_PRIMARY 	 = "\n\tDNS_PRIMARY = ";
			DNS_PRIMARY		 << fParentWindow->fPrimaryDNS->Text();
	BString DNS_SECONDARY	 = "\n\tDNS_SECONDARY = ";
			DNS_SECONDARY	 << fParentWindow->fSecondaryDNS->Text();
	BString ROUTER		     = "\n\tROUTER = ";//where to get that info ??
	BString VERSION		     = "\n\tVERSION = ";//where to get that info ??
	BString INTERFACES	     = "\n\tINTERFACES = ";//Unused but kept for background compatibility

	// Now let's see how many interfaces there are
	int32 numb = fParentWindow->fInterfacesList->CountItems();
	
	// Each interface has 8 fields
	BString fields[numb][8];
	
	for(int i=0;i<numb;i++){ //Let's fill the fields
		fields[i][0] = "\n\tDEVICECONFIG = "; //where to get that info ??
		fields[i][1] = "\n\tDEVICELINK = "; //where to get that info ??
		fields[i][2] = "\n\tDEVICETYPE = "; //where to get that info ??
		fields[i][3] = "\n\tIPADDRESS = ";
		fields[i][4] = "\n\tNETMASK = ";
		fields[i][5] = "\n\tPRETTYNAME = ";
		fields[i][6] = "\n\tENABLED = ";
		fields[i][7] = "\n\tDHCP = ";		
	}	
													
	for(int i=0;i<numb;i++){ //Now let's put the data
		NetListItem *item = dynamic_cast <NetListItem *> (fParentWindow->fInterfacesList->ItemAt(i)); 
		
		fields[i][3] << item->fIPADDRESS;
		fields[i][4] << item->fNETMASK;
		fields[i][5] << item->fPRETTYNAME;
		fields[i][6] << item->fENABLED;
		fields[i][7] << item->fDHCP;	  	
	}		
	
	// Now let's write all this stuff
	BString path;
	path = "/boot/home/config/settings/network";
	
	// This SaveBackup() function is used for backups(so a network.<name> file)
	// and for the "Save" button which uses the network file as the backup file. 
	if (bak_name != NULL)             
		path << "." << bak_name;
	else {
		BEntry entry(path.String());
		entry.Remove();
	}	
	
	FILE *bak_file = fopen(path.String(),"w"); 	 
	fprintf(bak_file,"GLOBAL:");		//The "GLOBAL" part
	fprintf(bak_file,DNS_DOMAIN.String());
	fprintf(bak_file,HOSTNAME.String());
	fprintf(bak_file,USERNAME.String());
	fprintf(bak_file,PROTOCOLS.String());
	fprintf(bak_file,PASSWORD.String());
	fprintf(bak_file,FTP_ENABLED.String());
	fprintf(bak_file,IP_FORWARD.String());		
	fprintf(bak_file,DNS_ENABLED.String());		
	fprintf(bak_file,TELNETD_ENABLED.String());		
	fprintf(bak_file,DNS_PRIMARY.String());		
	fprintf(bak_file,DNS_SECONDARY.String());	
	fprintf(bak_file,ROUTER.String());	
	fprintf(bak_file,VERSION.String());			
	fprintf(bak_file,INTERFACES.String());
	
	for (int i=0;i<numb;i++) {
		
		// The interfacexx parts
		BString interfacex = "\ninterface";
		interfacex << i << ":";
		fprintf(bak_file,interfacex.String());
		for (int j=0;j<8;j++)
			fprintf(bak_file,fields[i][j].String());
	}		

	fclose(bak_file);	
*/
}
