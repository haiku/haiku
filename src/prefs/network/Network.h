#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <ListItem.h>
#include <ListView.h>
#include <RadioButton.h>
#include <String.h>
#include <StringView.h>
#include <TabView.h>
#include <TextControl.h>
#include <View.h>
#include <Window.h>

#ifndef _NETWORK_H_
#define _NETWORK_H_
/*General notes:
	_ Generates two warnings when compiling which you don't have to care about. 	
	_ The way the net_server is restarted might not be very good as it takes 0.10s
		contrarely to Be's original ones that takes few seconds and does many more 
		things that I don't know yet. Will be improved ASAP.(As soon as I know how 
		tings are working...) 
	_ Custom button actually add an interface so I changed his label to "Add"
		which make it clearer.
	_ Custom/Add button does nothing because of next line.
	_ Don't know where to find the "interface types".	
	_ Clear button does nothing as I couldn't find his utility.
	_ Where to find the App's icon ? Ask Creative Design Team ?
	_ Is there any "OBOS header" to insert on top of each sheet?
*/	


class NetListView : public BListView 
{
	public:
		NetListView(BRect frame,const char *name);
		
		virtual void SelectionChanged(void);
};


class NetListItem : public BStringItem
{
	public:
		NetListItem(const char *text);
		
		BString fIPADDRESS;
		BString fNETMASK;
		BString fPRETTYNAME;
		int 	fENABLED;
		int		fDHCP;
};

class NetworkWindow : public BWindow
{
	public:
		NetworkWindow();
//The whole GUI which is hierarchized to see 'what is attached to what'
			BView 								*fview;
				BButton							*frestart;
				BButton							*frevert;
				BButton							*fsave;				
				BTabView 						*ftabview;
					BTab 						*fidentity;
						BView 					*fidentity_view;
							BBox 				*fnames;
								BTextControl 	*fdomain;
								BTextControl 	*fhost;
								BTextControl 	*fprimary_dns;
								BTextControl 	*fsecondary_dns;
							BBox 				*finterfaces;
								NetListView		*finterfaces_list;
								BButton			*fsettings;
								BButton			*fclear;
								BButton			*fcustom;
					BTab 						*fservices;
						BView 					*fservices_view;
							BCheckBox			*fftp_server;
							BCheckBox			*ftelnet_server;
							BCheckBox			*fapple_talk;
							BCheckBox			*fip_forwarding;
							BButton				*flogin_info;
							BBox				*fconfigurations;
								NetListView		*fconfigurations_list;
								BButton			*fbackup;
								BButton			*frestore;
								BButton			*fremove;

//Messages
		static const int32 kRestart_Networking_M = 'ranm' ;
		static const int32 kRevert_M			 = 'revm' ;
		static const int32 kSave_M				 = 'savm' ;
		static const int32 kSettings_M			 = 'setm' ;
		static const int32 kClear_M				 = 'clem' ;
		static const int32 kCustom_M			 = 'cusm' ;
		static const int32 kLogin_Info_M		 = 'logm' ;
		static const int32 kBackup_M			 = 'bakm' ;
		static const int32 kRestore_M			 = 'resm' ;
		static const int32 kDelete_M			 = 'delm' ;
		
		static const int32 kSOMETHING_HAS_CHANGED_M = 'shcm' ;
	
/*As this window is the 'main' one and that at startup it gathers up the
infos with the GetEverything() method, some data are kept here and will be passed
to who needs it. */
		BString fusername;
		BString flogin_info_S[2];	
	
		
		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage *message);	
				
		//All these are defined in funcs.cpp
		void Remove_Config_File();			
		void Get_Configs_List();												
		void GetEverything(const char *bakname = NULL); 		//Gets all the infos of the interfaces at startup, display and store them
};																//or get them from a backup file.

class BackupWin : public BWindow //The window that appears when you press 
{                                //the 'Backup' button
	public:
		BackupWin();	
		BView   			*fbakview;
			BTextControl 	*fbak_name;
			BButton 		*fbak_cancel;
			BButton			*fbak_done;	

		NetworkWindow *fparent_window;
		
		void Flatten_bak(const char *bak_name = NULL);	//Writes the backup or used	
														//when pressing 'save'
		virtual void MessageReceived(BMessage *message);	
		
		static const int32 kFlatten_bak_M		= 'fbam' ;		
		static const int32 kCancel_bak_M		= 'cbam' ;		
};

class LoginInfo : public BWindow //The window that appears when you press 
{                                //the 'Login Info...' button
	public:
		LoginInfo();	
		BView   			*flogin_info_view;
			BTextControl 	*flogin_info_name;
			BTextControl 	*flogin_info_password;
			BTextControl 	*flogin_info_confirm;
			BButton 		*flogin_info_cancel;
			BButton			*flogin_info_done;	

		NetworkWindow *fparent_window;
		
		virtual void MessageReceived(BMessage *message);	
		
		static const int32 kLogin_Info_Cancel_M  = 'licm' ;
		static const int32 kLogin_Info_Done_M	 = 'lidn' ;	
};

class InterfaceWin : public BWindow //The window that appears when you press 
{                                   //the 'Settings...' button.
	public:
		InterfaceWin();	
		BView			   	*finterview;
			BStringView		*finter_name;
			BCheckBox		*finter_enabled;
			BRadioButton 	*finter_dhcp;
			BRadioButton	*finter_manual;
			BButton 		*finter_config;
			BButton 		*finter_cancel;
			BButton			*finter_done;			
			BBox			*finter_box;
				BTextControl 	*finter_ip;
				BTextControl 	*finter_mask;
				BTextControl 	*finter_gateway;									

		NetworkWindow *fparent_window;
		bool fhas_something_changed;
					
		virtual void MessageReceived(BMessage *message);

		static const int32 kCancel_inter_M		  = 'caim' ;		
		static const int32 kDone_inter_M		  = 'doim' ;
		static const int32 kSOMETHING_HAS_CHANGED = 'sohc' ;					
};

class RestartWindow : public BWindow
{
	public:
		RestartWindow();
			BView			*fview;
				BStringView *frestarting;
				BStringView *fwhatshappening;	
};

class Network : public BApplication
{
	public:
		Network();
		
		NetworkWindow *fwindow;
}; 

#endif
