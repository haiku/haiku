#ifndef NETWORK_WINDOW_H
#define NETWORK_WINDOW_H

#include <Window.h>
#include <View.h>
#include <Button.h>
#include <TabView.h>
#include <Box.h>
#include <CheckBox.h>
#include <String.h>
#include "ConfigData.h"

class NetListView;

class NetworkWindow : public BWindow
{
public:
	NetworkWindow();
	
	BView 			*fView;
	BTabView 		*fTabView;
	
	BButton			*fRestart;
	BButton			*fRevert;
	BButton			*fSave;
		
	BTab 			*fIdentity;
	BView 			*fIdentityView;
	
	BBox 			*fNames;
	BTextControl 	*fDomain;
	BTextControl 	*fHost;
	BTextControl 	*fPrimaryDNS;
	BTextControl 	*fSecondaryDNS;
	
	BBox 			*fInterfaces;
	NetListView		*fInterfacesList;
	BButton			*fSettings;
	BButton			*fClear;
	BButton			*fCustom;
	
	BTab 			*fServices;
	BView 			*fServicesView;
	BCheckBox		*fFTPServer;
	BCheckBox		*fTelnetServer;
	BCheckBox		*fAppleTalk;
	BCheckBox		*fIPForwarding;
	BButton			*fLoginInfo;
	
	BBox			*fConfigurations;
	NetListView		*fConfigurationsList;
	BButton			*fBackup;
	BButton			*fRestore;
	BButton			*fRemove;

	// Messages
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
	
/*
	As this window is the 'main' one and that at startup it gathers up the
	info with the LoadSettings() method, some data are kept here and will be passed
	to who needs it.
*/
	BString fusername;
	BString flogin_info_S[2];
		
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *message);	
				
	//All these are defined in funcs.cpp
	void DeleteConfigFile();			
	void LoadConfigEntries();												
	void LoadSettings(const char *bakname = NULL); 		
		
	// Gets all the infos of the interfaces at startup, display and store them
	// or get them from a backup file.
	ConfigData fData;
};		

#endif
