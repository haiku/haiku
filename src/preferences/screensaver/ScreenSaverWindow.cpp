#include "ScreenSaverWindow.h"
#include <ListView.h>
#include <Application.h>
#include <Button.h>
#include <Box.h>
#include <Font.h>
#include <TabView.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <ScrollView.h>
#include <FindDirectory.h>
#include <Slider.h>
#include <StringView.h>
#include <ScreenSaver.h>
#include "MouseAreaView.h"
#include "PreviewView.h"
#include <Roster.h>
#include <stdio.h>

const int32 zero=0;

void drawPositionalMonitor(BView *view,BRect areaToDrawIn,int state);
BView *drawSampleMonitor(BView *view, BRect area);


int 
secondsToSlider(int val) 
{
	int count=sizeof(kTimeInSeconds)/sizeof(int);
	for (int t=0;t<count;t++)
		if (kTimeInSeconds[t]==val)
				return t;
	return -1;
}



struct SSListItem {
	BString fileName;
	BString displayName;	
};

void 
ScreenSaverWin::SaverSelected(void) 
{
	if (fListView1->CurrentSelection()>=0) {
		SSListItem* listItem;
		if (fPreviewDisplay->ScreenSaver())
			fPreviewDisplay->ScreenSaver()->StopConfig();
	   	listItem = reinterpret_cast<SSListItem*>(fAddonList->ItemAt(fListView1->CurrentSelection()));
		BString settingsMsgName(listItem->fileName);
		fPreviewDisplay->SetScreenSaver(settingsMsgName);
		if (fSettingsArea) {
			fModuleSettingsBox->RemoveChild(fSettingsArea);
			delete fSettingsArea;
		}
		BRect bnds=fModuleSettingsBox->Bounds();
		bnds.InsetBy(5,10);
		fSettingsArea=new BView(bnds,"settingsArea",B_FOLLOW_NONE,B_WILL_DRAW);
		fSettingsArea->SetViewColor(216,216,216);
		fModuleSettingsBox->AddChild(fSettingsArea);
			
		if (fPreviewDisplay->ScreenSaver())
			fPreviewDisplay->ScreenSaver()->StartConfig(fSettingsArea);
		}
}


void 
ScreenSaverWin::MessageReceived(BMessage *msg) 
{
	switch(msg->what) {
		case kPwbutton: 
			fPwMessenger->SendMessage(kShow);
      		break;
		case B_QUIT_REQUESTED:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		case kSaver_sel: 
			SaverSelected();
			break;
		case kTest_btn:
			be_roster->Launch("application/x-vnd.OBOS-ScreenSaverApp",fPrefs.GetSettings());
			break;
		case kAdd_btn:
			fFilePanel->Show();
			break;
		case kUpdatelist:
			populateScreenSaverList();
			break;
  	}
	updateStatus(); // This could get called sometimes when it doesn't need to. Shouldn't hurt
	BWindow::MessageReceived(msg);
}


bool 
ScreenSaverWin::QuitRequested() 
{
	updateStatus();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}       


void 
ScreenSaverWin::updateStatus(void) 
{
	DisableUpdates();
	// Policy - enable and disable controls as per checkboxes, etc
	fPasswordCheckbox->SetEnabled(fEnableCheckbox->Value());
	fTurnOffScreenCheckBox->SetEnabled(fEnableCheckbox->Value());
	fRunSlider->SetEnabled(fEnableCheckbox->Value());
	fTurnOffSlider->SetEnabled(fEnableCheckbox->Value() && fTurnOffScreenCheckBox->Value());
	fTurnOffSlider->SetEnabled(false); // This never seems to turn on in the R5 version
	fTurnOffScreenCheckBox->SetEnabled(false);
	fPasswordSlider->SetEnabled(fEnableCheckbox->Value() && fPasswordCheckbox->Value());
	fPasswordButton->SetEnabled(fEnableCheckbox->Value() && fPasswordCheckbox->Value());
	// Set the labels for the sliders
	fRunSlider->SetLabel(kTimes[fRunSlider->Value()]);
	fTurnOffSlider->SetLabel(kTimes[fTurnOffSlider->Value()]);
	fPasswordSlider->SetLabel(kTimes[fPasswordSlider->Value()]);
	EnableUpdates();
	// Update the saved preferences
	fPrefs.SetWindowFrame(Frame());	
	fPrefs.SetWindowTab(fTabView->Selection());
	fPrefs.SetTimeFlags(fEnableCheckbox->Value());
	fPrefs.SetBlankTime(kTimeInSeconds[fRunSlider->Value()]);
	fPrefs.SetOffTime(kTimeInSeconds[fTurnOffSlider->Value()]);
	fPrefs.SetSuspendTime(kTimeInSeconds[fTurnOffSlider->Value()]);
	fPrefs.SetStandbyTime(kTimeInSeconds[fTurnOffSlider->Value()]);
	fPrefs.SetBlankCorner(fFadeNow->getDirection());
	fPrefs.SetNeverBlankCorner(fFadeNever->getDirection());
	fPrefs.SetLockEnable(fPasswordCheckbox->Value());
	fPrefs.SetPasswordTime(kTimeInSeconds[fPasswordSlider->Value()]);
	int selection=fListView1->CurrentSelection(0);
	if (selection>=0)
		fPrefs.SetModuleName(((BStringItem *)(fListView1->ItemAt(selection)))->Text());
// TODO - Tell the password window to update its stuff
	BMessage ssState;
	if ((fPreviewDisplay->ScreenSaver()) && (fPreviewDisplay->ScreenSaver()->SaveState(&ssState)==B_OK))
		fPrefs.SetState(&ssState);
	fPrefs.SaveSettings();
};


void 
ScreenSaverWin::SetupForm(void) 
{
	fFilePanel=new BFilePanel();

	BRect r;
	BView *background;
	BTab *tab;
	r = Bounds();
	
// Create a background view
	background=new BView(r,"background",B_FOLLOW_NONE,0);
	background->SetViewColor(216,216,216,0);
	AddChild(background);

// Add the tab view to the background
	r.InsetBy(0,3);
	fTabView = new BTabView(r, "tab_view");
	fTabView->SetViewColor(216,216,216,0);
	r = fTabView->Bounds();
	r.InsetBy(0,4);
	r.bottom -= fTabView->TabHeight();

// Time to load the settings into a message and implement them...
	fPrefs.LoadSettings();

	tab = new BTab();
	fTabView->AddTab(fTab2=new BView(r,"Fade",B_FOLLOW_NONE,0), tab);
	tab->SetLabel("Fade");

	tab = new BTab();
	fTabView->AddTab(fTab1=new BView(r,"Modules",B_FOLLOW_NONE,0), tab);
	tab->SetLabel("Modules");
	background->AddChild(fTabView);

// Create the controls inside the tabs
	setupTab2();
	setupTab1();

// Create the password editing window
	fPwWin=new pwWindow;
	fPwMessenger=new BMessenger (NULL,fPwWin);
	fPwWin->Run();

	MoveTo(fPrefs.WindowFrame().left,fPrefs.WindowFrame().top);
	ResizeTo(fPrefs.WindowFrame().right-fPrefs.WindowFrame().left,fPrefs.WindowFrame().bottom-fPrefs.WindowFrame().top);
	fTabView->Select(fPrefs.WindowTab());
	fEnableCheckbox->SetValue(fPrefs.TimeFlags());
	fRunSlider->SetValue(secondsToSlider(fPrefs.BlankTime()));
	fTurnOffSlider->SetValue(secondsToSlider(fPrefs.OffTime()));
	fFadeNow->setDirection(fPrefs.GetBlankCorner());
	fFadeNever->setDirection(fPrefs.GetNeverBlankCorner());
	fPasswordCheckbox->SetValue(fPrefs.LockEnable());
	fPasswordSlider->SetValue(secondsToSlider(fPrefs.PasswordTime()));
	const BStringItem **ptr = (const BStringItem **)(fListView1->Items());
	long count=fListView1->CountItems();
	if (fPrefs.ModuleName() && ptr) 	
		for ( long i = 0; i < count; i++ ) {
			if (BString(fPrefs.ModuleName())==((*ptr++)->Text())) {
				fListView1->Select(count=i); // Clever bit here - intentional assignment.
				SaverSelected();
				fListView1->ScrollToSelection();
			}
		}
	updateStatus();
} 


// Set the common Look and Feel stuff for a given control
void 
commonLookAndFeel(BView *widget,bool isSlider,bool isControl) 
{
  	{rgb_color clr = {216,216,216,255}; widget->SetViewColor(clr);}
	if (isSlider) {
		BSlider *slid=dynamic_cast<BSlider *>(widget);
  		{rgb_color clr = {160,160,160,0}; slid->SetBarColor(clr);}
  		slid->SetHashMarks(B_HASH_MARKS_NONE);
  		slid->SetHashMarkCount(0);
  		slid->SetStyle(B_TRIANGLE_THUMB);
  		slid->SetLimitLabels("","");
  		slid->SetLimitLabels(NULL,NULL);
  		slid->SetLabel("0 minutes");
  		slid->SetValue(0);
		slid->SetEnabled(true);
	}
  	{rgb_color clr = {0,0,0,0}; widget->SetHighColor(clr);}
  	widget->SetFlags(B_WILL_DRAW|B_NAVIGABLE);
  	widget->SetResizingMode(B_FOLLOW_NONE);
  	widget->SetFontSize(10);
  	widget->SetFont(be_plain_font);
	if (isControl) {
		BControl *wid=dynamic_cast<BControl *>(widget);
		wid->SetEnabled(true);
	}
}


// Iterate over a directory, adding the directories files to the list
void 
addScreenSaversToList (directory_which dir, BList *list) 
{
	BPath path;
	find_directory(dir,&path);
	path.Append("Screen Savers",true);
  
	const char* pathName = path.Path();
  
	BDirectory ssDir(pathName);
	BEntry thisSS;
	char thisName[B_FILE_NAME_LENGTH];

	while (B_OK==ssDir.GetNextEntry(&thisSS,true)) {
		thisSS.GetName(thisName);
		SSListItem* tempListItem = new SSListItem;
		tempListItem->fileName = pathName;
		tempListItem->fileName += "/";
		tempListItem->fileName += thisName;
		tempListItem->displayName = thisName;
	
		list->AddItem(tempListItem); 
	}
}


// sorting function for SSListItems
int 
compareSSListItems(const void* left, const void* right) 
{
	SSListItem* leftItem  = *(SSListItem **)left;
	SSListItem* rightItem = *(SSListItem **)right;
  
	return leftItem->displayName.Compare(rightItem->displayName);
}


void 
ScreenSaverWin::populateScreenSaverList(void) 
{
	if (!fAddonList)
		fAddonList = new BList;
	else 
		for (void *i=fAddonList->RemoveItem(zero);i;i=fAddonList->RemoveItem(zero)) 
			delete ((SSListItem *)i);

	SSListItem* tempListItem = new SSListItem;
	tempListItem->fileName = "";
	tempListItem->displayName = "Blackness";
  	fAddonList->AddItem(tempListItem); 
	addScreenSaversToList( B_BEOS_ADDONS_DIRECTORY, fAddonList );
	addScreenSaversToList( B_USER_ADDONS_DIRECTORY, fAddonList );
	fAddonList->SortItems(compareSSListItems);
  
// Add the strings in the BList to a BListView
 	fListView1->DeselectAll(); 
	for (void *i=fListView1->RemoveItem(zero);i;i=fListView1->RemoveItem(zero)) 
		delete ((BStringItem *)i);

	int numItems = fAddonList->CountItems();
	for( int i = 0; i < numItems; ++i ) {
		SSListItem* item = (SSListItem*)(fAddonList->ItemAt(i));
		fListView1->AddItem( new BStringItem(item->displayName.String()) );
	}
}


// Create the controls for the first tab
void 
ScreenSaverWin::setupTab1(void) 
{
	int columns[4]={15,150,180,430};
	int rows[6]={15,120,135,255,263,280};

	{rgb_color clr = {216,216,216,255}; fTab1->SetViewColor(clr);}
	fTab1->AddChild( fModuleSettingsBox = new BBox(BRect(columns[2],rows[0],columns[3],rows[5]),"ModuleSettingsBox",B_FOLLOW_NONE,B_WILL_DRAW));
	fModuleSettingsBox->SetLabel("Module settings");
	fModuleSettingsBox->SetBorder(B_FANCY_BORDER);

	fListView1 = new BListView(BRect(columns[0],rows[2],columns[1]+3,rows[3]),"ListView1",B_SINGLE_SELECTION_LIST);
	fTab1->AddChild(new BScrollView("scroll_list",fListView1,B_FOLLOW_NONE,0,false,true));
	commonLookAndFeel(fModuleSettingsBox,false,false);
	{rgb_color clr = {255,255,255,0}; fListView1->SetViewColor(clr);}
	fListView1->SetListType(B_SINGLE_SELECTION_LIST);

	// selection message for screensaver list
	fListView1->SetSelectionMessage( new BMessage( kSaver_sel ) );
  
	fTab1->AddChild( fTestButton = new BButton(BRect(columns[0],rows[4],91,rows[5]),"TestButton","Test", new BMessage (kTest_btn)));
	commonLookAndFeel(fTestButton,false,true);
	fTestButton->SetLabel("Test");

	fTab1->AddChild( fAddButton = new BButton(BRect(97,rows[4],columns[2]-10,rows[5]),"AddButton","Add...", new BMessage (kAdd_btn)));
	commonLookAndFeel(fAddButton,false,true);
	fAddButton->SetLabel("Add...");

	fTab1->AddChild(fPreviewDisplay = new PreviewView(BRect(columns[0]+5,rows[0],columns[1],rows[1]),"preview",&fPrefs));
 	populateScreenSaverList(); 
} 


// Create the controls for the second tab
void 
ScreenSaverWin::setupTab2(void) 
{
	font_height stdFontHt;
	be_plain_font->GetHeight(&stdFontHt);  
	int stringHeight=(int)(stdFontHt.ascent+stdFontHt.descent),sliderHeight=30;
	int topEdge;
	{rgb_color clr = {216,216,216,255}; fTab2->SetViewColor(clr);}
	fTab2->AddChild( fEnableScreenSaverBox = new BBox(BRect(11,13,437,280),"EnableScreenSaverBox"));
	commonLookAndFeel(fEnableScreenSaverBox,false,false);

	fEnableCheckbox = new BCheckBox(BRect(0,0,90,stringHeight),"EnableCheckBox","Enable Screen Saver", new BMessage (kTab2_chg));
	fEnableScreenSaverBox->SetLabel(fEnableCheckbox);
	fEnableScreenSaverBox->SetBorder(B_FANCY_BORDER);

	// Run Module
	topEdge=26;
	fEnableScreenSaverBox->AddChild( fStringView1 = new BStringView(BRect(21,topEdge,101,topEdge+stringHeight),"StringView1","Run module"));
	commonLookAndFeel(fStringView1,false,false);
	fStringView1->SetText("Run module");
	fStringView1->SetAlignment(B_ALIGN_LEFT);

	fEnableScreenSaverBox->AddChild( fRunSlider = new BSlider(BRect(132,topEdge,415,topEdge+sliderHeight),"RunSlider","minutes", new BMessage(kTab2_chg), 0, 25));
	fRunSlider->SetModificationMessage(new BMessage(kTab2_chg));
	commonLookAndFeel(fRunSlider,true,true);
	float w,h;
	fRunSlider->GetPreferredSize(&w,&h);
	sliderHeight=(int)h;

	// Turn Off
	topEdge+=sliderHeight;
	fEnableScreenSaverBox->AddChild( fTurnOffScreenCheckBox = new BCheckBox(BRect(9,topEdge,107,topEdge+stringHeight),"TurnOffScreenCheckBox","Turn off screen",  new BMessage (kTab2_chg)));
	commonLookAndFeel(fTurnOffScreenCheckBox,false,true);
	fTurnOffScreenCheckBox->SetLabel("Turn off screen");
	fTurnOffScreenCheckBox->SetResizingMode(B_FOLLOW_NONE);

	fEnableScreenSaverBox->AddChild( fTurnOffSlider = new BSlider(BRect(132,topEdge,415,topEdge+sliderHeight),"TurnOffSlider","", new BMessage(kTab2_chg), 0, 25));
	fTurnOffSlider->SetModificationMessage(new BMessage(kTab2_chg));
	commonLookAndFeel(fTurnOffSlider,true,true);

	// Password
	topEdge+=sliderHeight;
	fEnableScreenSaverBox->AddChild( fPasswordCheckbox = new BCheckBox(BRect(9,topEdge,108,topEdge+stringHeight),"PasswordCheckbox","Password lock",  new BMessage (kTab2_chg)));
	commonLookAndFeel(fPasswordCheckbox,false,true);
	fPasswordCheckbox->SetLabel("Password lock");

	fEnableScreenSaverBox->AddChild( fPasswordSlider = new BSlider(BRect(132,topEdge,415,topEdge+sliderHeight),"PasswordSlider","", new BMessage(kTab2_chg), 0, 25));
	fPasswordSlider->SetModificationMessage(new BMessage(kTab2_chg));
	commonLookAndFeel(fPasswordSlider,true,true);

	topEdge+=sliderHeight;
	fEnableScreenSaverBox->AddChild( fPasswordButton = new BButton(BRect(331,topEdge,405,topEdge+25),"PasswordButton","Password...",  new BMessage (kPwbutton)));
	commonLookAndFeel(fPasswordButton,false,true);
	fPasswordButton->SetLabel("Password...");

	// Bottom

	fEnableScreenSaverBox->AddChild(fFadeNow=new MouseAreaView(BRect(20,205,80,260),"fadeNow"));
	fEnableScreenSaverBox->AddChild(fFadeNever=new MouseAreaView(BRect(220,205,280,260),"fadeNever"));

	fEnableScreenSaverBox->AddChild( fFadeNowString = new BStringView(BRect(85,210,188,222),"FadeNowString","Fade now when"));
	commonLookAndFeel(fFadeNowString,false,false);
	fFadeNowString->SetText("Fade now when");
	fFadeNowString->SetAlignment(B_ALIGN_LEFT);

	fEnableScreenSaverBox->AddChild( fFadeNowString2 = new BStringView(BRect(85,225,188,237),"FadeNowString2","mouse is here"));
	commonLookAndFeel(fFadeNowString2,false,false);
	fFadeNowString2->SetText("mouse is here");
	fFadeNowString2->SetAlignment(B_ALIGN_LEFT);

	fEnableScreenSaverBox->AddChild( fDontFadeString = new BStringView(BRect(285,210,382,222),"DontFadeString","Don't fade when"));
	commonLookAndFeel(fDontFadeString,false,false);
	fDontFadeString->SetText("Don't fade when");
	fDontFadeString->SetAlignment(B_ALIGN_LEFT);

	fEnableScreenSaverBox->AddChild( fDontFadeString2 = new BStringView(BRect(285,225,382,237),"DontFadeString2","mouse is here"));
	commonLookAndFeel(fDontFadeString2,false,false);
	fDontFadeString2->SetText("mouse is here");
	fDontFadeString2->SetAlignment(B_ALIGN_LEFT);
}
