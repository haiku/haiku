/*

DUNWindow Header - DialUp Networking BWindow Header

Authors: Sikosis (beos@gravity24hr.com)
		 Misza (misza@ihug.com.au) 

(C) 2002 OpenBeOS under MIT license

*/

#ifndef __DUNWINDOW_H__
#define __DUNWINDOW_H__

#include <Application.h>
#include <Alert.h>
#include <FindDirectory.h>  // "Be.h doesn't compile on my system" - Sikosis
#include <Entry.h>
#include <File.h>
#include <Font.h>
#include <Path.h>
#include <stdio.h>
#include <iostream>
#include <InterfaceKit.h>
#include "ModemWindow.h"
#include "DUNView.h"
#include "NewConnectionWindow.h"
#include "SettingsWindow.h"
#include "DetailsView.h"
#include "TreeView.h"
#include "LocationView.h"

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

const uint32 TEST_BOX_1 = 'Tb1';
const uint32 TEST_BOX_2 = 'Tb2';

const uint32 ADD_NEW_CONNECTION = 'adnc';

//const char *kConnectionName = "ConnectionName";

struct last_active { int x;int y;};
class DUNView; 

class DUNWindow : public BWindow {
public:
   DUNWindow(BRect frame);
   ~DUNWindow();
   virtual bool QuitRequested();
   virtual void MessageReceived(BMessage *message);
   
   void LoadSettings(BMessage *msg);
   void SaveSettings();
   //these functions handle the resizing of the window
   void SetStatus(int m,int n);
   void InvalidateAll();
   void DoResizeMagic();
   void DUNResizeHandler();
private:
	ModemWindow *modemWindow;
    void InitWindow();
    DUNView* aDUNview;
    
    NewConnectionWindow* ptrNewConnectionWindow;
    SettingsWindow    *settingsWindow;
    BBox              *topframe;
    BBox              *middleframe;
    BBox              *bottomframe;
   
    BButton           *modembutton;
    BButton           *disconnectbutton;
    BButton           *connectbutton;
   
    BOutlineListView  *connectionlistitem;
    BOutlineListView  *locationlistitem;
   
    BMenuField        *connectionmenufield;
    BMenuField        *locationmenufield;
   
    BMenu             *conmenufield;
    BMenu             *locmenufield;
    
    TreeView*         trvconnect;
    TreeView*         trvlocation;
    
    BView*            middle_frame_label;
    BView*            top_frame_label;

    struct            last_active last;
    
    BStringView*      passmeon;//what was tvConnectionProfile
    DetailsView*      passmeon2;
    LocationView*     passmeon3;
    BStringView*      passmeon4;//what was tvCallWaiting
    
    BStringView*      svNoConnect;
    BStringView*      svLocalIPAddress;
    BStringView*      svTimeOnline;
    BStringView*      svLIP;
};

#endif
