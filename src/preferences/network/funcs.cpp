#include <stdio.h>
#include <stdlib.h>
#include <Alert.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>

#include "Network.h"

#define B_GREY 216,216,216


void BackupWin::Flatten_bak(const char *bak_name)
{
/*To understand what follows, u'd better open the 
 /boot/home/config/settings/network textfile */

//GLOBAL:
	BString DNS_DOMAIN  	 = "\n\tDNS_DOMAIN = ";
			DNS_DOMAIN       << fparent_window->fdomain->Text();
	BString HOSTNAME    	 = "\n\tHOSTNAME = ";
			HOSTNAME         << fparent_window->fhost->Text();
	BString USERNAME  	     = "\n\tUSERNAME = ";
			USERNAME		 << fparent_window->flogin_info_S[0];
	BString PROTOCOLS 	     = "\n\tPROTOCOLS = ";/*where to get that info ??*/
	BString PASSWORD		 = "\n\tPASSWORD = ";
			PASSWORD         << fparent_window->flogin_info_S[1];
	BString FTP_ENABLED		 = "\n\tFTP_ENABLED = "; 
		if (fparent_window->fftp_server->Value() == 0)
			     FTP_ENABLED << 0; 
		else     FTP_ENABLED << 1;
	BString IP_FORWARD 		 = "\n\tIP_FORWARD = ";
		if (fparent_window->fip_forwarding->Value() == 0)
			      IP_FORWARD << 0;
		else      IP_FORWARD << 1;
	BString DNS_ENABLED	     = "\n\tDNS_ENABLED = ";/*where to get that info ??*/
	BString TELNETD_ENABLED  = "\n\tTELNETD_ENABLED = ";
		if (fparent_window->ftelnet_server->Value() == 0)
			 TELNETD_ENABLED << 0;
		else TELNETD_ENABLED << 1;
	BString DNS_PRIMARY 	 = "\n\tDNS_PRIMARY = ";
			DNS_PRIMARY		 << fparent_window->fprimary_dns->Text();
	BString DNS_SECONDARY	 = "\n\tDNS_SECONDARY = ";
			DNS_SECONDARY	 << fparent_window->fsecondary_dns->Text();
	BString ROUTER		     = "\n\tROUTER = ";/*where to get that info ??*/
	BString VERSION		     = "\n\tVERSION = ";/*where to get that info ??*/
	BString INTERFACES	     = "\n\tINTERFACES = ";/*Unused but kept for background compatibility*/

//Now let's see how many interfaces there are
	int32 numb = fparent_window->finterfaces_list->CountItems();
	BString fields[numb][8];  //Each interface has 8 fields
	int i;
	for (i=0;i<numb;i++){ //Let's fill the fields
		fields[i][0] = "\n\tDEVICECONFIG = "; /*where to get that info ??*/
		fields[i][1] = "\n\tDEVICELINK = "; /*where to get that info ??*/
		fields[i][2] = "\n\tDEVICETYPE = "; /*where to get that info ??*/
		fields[i][3] = "\n\tIPADDRESS = ";
		fields[i][4] = "\n\tNETMASK = ";
		fields[i][5] = "\n\tPRETTYNAME = ";
		fields[i][6] = "\n\tENABLED = ";
		fields[i][7] = "\n\tDHCP = ";		
	}	
													
	for (i=0;i<numb;i++){ //Now let's put the data
		NetListItem *item = dynamic_cast <NetListItem *> (fparent_window->finterfaces_list->ItemAt(i)); 
		
		fields[i][3] << item->fIPADDRESS;
		fields[i][4] << item->fNETMASK;
		fields[i][5] << item->fPRETTYNAME;
		fields[i][6] << item->fENABLED;
		fields[i][7] << item->fDHCP;	  	
	}		
	
//Now let's write all these stuffs	
	BString path;
	path = "/boot/home/config/settings/network";
	
	if (bak_name != NULL)             //This Flatten_bak() function is used
		path<<"."<<bak_name;          //for backuping (so a network.<name> file)	
	else {					          //and for the "Save" button which uses the 
		BEntry entry(path.String());  //network file as the backup file. 
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
	
	for (int i=0;i<numb;i++){		//The interfacexx parts
		BString interfacex = "\ninterface";
		interfacex << i << ":";
		fprintf(bak_file,interfacex.String());
		for (int j=0;j<8;j++){
			fprintf(bak_file,fields[i][j].String());
		}
	}		

	fclose(bak_file);	
}

void NetworkWindow::Remove_Config_File()
{
	BAlert *alert = new BAlert("Alert","Really delete networking configuration ?","  Delete  ","  Cancel  ",NULL,B_WIDTH_FROM_WIDEST,B_INFO_ALERT);
	int32 result = alert->Go();
	if (result == 0){
		BStringItem *item = dynamic_cast <BStringItem *> (fconfigurations_list->ItemAt(fconfigurations_list->CurrentSelection()));	
	
		BString path;
		path = "/boot/home/config/settings/network.";
		path<< item->Text();

		BEntry entry(path.String());
		entry.Remove();
	}						
}

void NetworkWindow::Get_Configs_List()
{
	fconfigurations_list->MakeEmpty();
	
	BDirectory settings("/boot/home/config/settings/");
	BEntry entry;

	if (settings.InitCheck() == B_OK){	
		while (settings.GetNextEntry(&entry) == B_OK){				
			char buffer[9];  		//All the config files start with "network."  
			entry.GetName(buffer);  //so we only get the 8 first characters
			buffer[8]='\0';         //to compare.

			if (strcmp(buffer,"network.") == 0){				
				BString string;
				char name[B_FILE_NAME_LENGTH];
				entry.GetName(name);          
				string<<name;
				string = string.Remove(0,8);				
				
				BStringItem *item = new BStringItem(string.String());
				fconfigurations_list->AddItem(item);
				}
			}
		}
}

void NetworkWindow::GetEverything(const char *bakname)
{
//The configuration is saved in the /boot/home/config/settings/network
//text file so the config is static. Maybe we could switch to dynamic by
//directly asking the net_server how things are
	
	if (bakname == NULL 
	    && BEntry("/boot/home/config/settings/network").Exists() == false){
		BAlert *alert = new BAlert("Network Alert",
		"Your network config file doesn't exists.\n"
		"It should be located at/boot/home/config/settings/network "
		,"Oups...",NULL,NULL,B_WIDTH_FROM_WIDEST,B_INFO_ALERT);
		alert->Go();
	}
	else {
/*This function being used when pressing "Revert" so first we need to
empty things before setting the new values*/
		Lock();
		finterfaces_list->MakeEmpty();	
		fftp_server	  ->SetValue(0);
		ftelnet_server ->SetValue(0);
		fapple_talk	  ->SetValue(0);
		fip_forwarding ->SetValue(0);
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
	
		fdomain		   ->SetText(GLOBAL[0].String()); //The BTextControls
		fhost  		   ->SetText(GLOBAL[1].String());
		fprimary_dns   ->SetText(GLOBAL[9].String());
		fsecondary_dns ->SetText(GLOBAL[10].String());
	
		if (GLOBAL[5]== "1") fftp_server->SetValue(1);    //Now the BCheckBoxes
		if (GLOBAL[6]== "1") fip_forwarding->SetValue(1); 
		if (GLOBAL[8]== "1") ftelnet_server->SetValue(1);
		
		for (i=0;i<(unsigned)number_of_interfaces;i++){ //Now the interfaces list
			BString string = INTERFACES[i][3];
			string<<"->"<<INTERFACES[i][5]<<"->"<<"State(Not implemented yet)";
			NetListItem *item = new NetListItem(string.String());
		
			item->fIPADDRESS  = INTERFACES[i][3];
			item->fNETMASK	  = INTERFACES[i][4];
			item->fPRETTYNAME = INTERFACES[i][5];
			item->fENABLED 	  = atoi(INTERFACES[i][6].String());   		
			item->fDHCP		  = atoi(INTERFACES[i][7].String());
		
			finterfaces_list->AddItem(item);
		}	
	
		Unlock(); 

//And save some of them
		fusername = GLOBAL[2];
	}	
}
