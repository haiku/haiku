/*
 * Copyright 2003-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *              Michael Phipps
 *              Jérôme Duval, jerome.duval@free.fr
 */

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
#include <Roster.h>
#include <stdio.h>

#include "ScreenSaverWindow.h"
#include "ScreenSaverItem.h"
#include "MouseAreaView.h"
#include "PreviewView.h"

#include <Debug.h>
#define CALLED() PRINT(("%s\n", __PRETTY_FUNCTION__))

static const char *kTimes[]={"30 seconds", "1 minute",             "1 minute 30 seconds",
                                                        "2 minutes",  "2 minutes 30 seconds", "3 minutes",
                                                        "4 minutes",  "5 minutes",            "6 minutes",
                                                        "7 minutes",  "8 minutes",            "9 minutes",
                                                        "10 minutes", "15 minutes",           "20 minutes",
                                                        "25 minutes", "30 minutes",           "40 minutes",
                                                        "50 minutes", "1 hour",               "1 hour 30 minutes",
                                                        "2 hours",    "2 hours 30 minutes",   "3 hours",
                                                        "4 hours",    "5 hours"};

static const int32 kTimeInUnits[]={     30,    60,   90,
                                                                        120,   150,  180,
                                                                        240,   300,  360,
                                                                        420,   480,  540,
                                                                        600,   900,  1200,
                                                                        1500,  1800, 2400,
                                                                        3000,  3600, 5400,
                                                                        7200,  9000, 10800,
                                                                        14400, 18000};

int32
UnitsToSlider(bigtime_t val) 
{
	int count = sizeof(kTimeInUnits)/sizeof(int);
	for (int t=0; t<count; t++)
		if (kTimeInUnits[t]*1000000LL == val)
			return t;
	return -1;
}


class CustomTab : public BTab
{
public:
	CustomTab(ScreenSaverWin *win, BView *view = NULL);
	virtual ~CustomTab();
	virtual void Select(BView *view);
private:
	ScreenSaverWin *fWin;
};


CustomTab::CustomTab(ScreenSaverWin *win, BView *view)
	: BTab(view),
	fWin(win)
{

}

CustomTab::~CustomTab()
{
}


void
CustomTab::Select(BView *view)
{
	BTab::Select(view);
	fWin->SelectCurrentModule();
}


ScreenSaverWin::ScreenSaverWin() 
	: BWindow(BRect(50,50,496,375),"Screen Saver",
		B_TITLED_WINDOW,B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_RESIZABLE) ,
	fFadeState(0),fNoFadeState(0), fListView1(NULL),
	fSelectedAddonFileName(NULL), 
	fCurrentAddon(NULL), fTestButton(NULL),
	fEnableCheckbox(NULL),
	fTurnOffMinutes(NULL), fRunMinutes(NULL),
	fPasswordMinutes(NULL), fPasswordButton(NULL),
	fFadeNowString(NULL),
	fFadeNowString2(NULL),
	fDontFadeString(NULL), fDontFadeString2(NULL),
	fFadeNow(NULL),fFadeNever(NULL),
	fPwMessenger(NULL), fFilePanel(NULL),
	fSettingsArea(NULL)

{
	SetupForm();
}


ScreenSaverWin::~ScreenSaverWin()
{
	delete fFilePanel;
}


void 
ScreenSaverWin::SaverSelected() 
{
	CALLED();
	UpdateStatus();
	fPrefs.SaveSettings();
	if (fListView1->CurrentSelection()>=0) {
		if (fPreviewDisplay->ScreenSaver())
			fPreviewDisplay->ScreenSaver()->StopConfig();
	   	ScreenSaverItem* item = reinterpret_cast<ScreenSaverItem*>(fListView1->ItemAt(fListView1->CurrentSelection()));
		BString settingsMsgName(item->Path());
		fPreviewDisplay->SetScreenSaver(settingsMsgName);
		if (fSettingsArea) {
			fModuleSettingsBox->RemoveChild(fSettingsArea);
			delete fSettingsArea;
		}
		BRect bnds = fModuleSettingsBox->Bounds();
		bnds.left = 3;
		bnds.right -= 7;
		bnds.top = 21;
		bnds.bottom -= 2;
		fSettingsArea = new BView(bnds,"settingsArea",B_FOLLOW_NONE,B_WILL_DRAW);
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
		case kRunSliderChanged:
			fRunSlider->SetLabel(kTimes[fRunSlider->Value()]);
			if (fRunSlider->Value()>fPasswordSlider->Value()) {
				fTurnOffSlider->SetValue(fRunSlider->Value());
				fTurnOffSlider->SetLabel(kTimes[fTurnOffSlider->Value()]);
				fPasswordSlider->SetValue(fRunSlider->Value());
				fPasswordSlider->SetLabel(kTimes[fPasswordSlider->Value()]);
			}
			break;
		case kTurnOffSliderChanged:
			fTurnOffSlider->SetLabel(kTimes[fTurnOffSlider->Value()]);
			break;
		case kPasswordSliderChanged:
			fPasswordSlider->SetLabel(kTimes[fPasswordSlider->Value()]);
			if (fPasswordSlider->Value()<fRunSlider->Value()) {
				fRunSlider->SetValue(fPasswordSlider->Value());
				fRunSlider->SetLabel(kTimes[fRunSlider->Value()]);
			}
			break;
		case kPasswordCheckbox:
		case kEnableScreenSaverChanged:
			UpdateStatus();
			break;
		case kPwbutton: 
			fPwMessenger->SendMessage(kShow);
      			break;
		case kSaver_sel: 
			SaverSelected();
			break;
		case kTest_btn:
		{
			BMessage &settings = fPrefs.GetSettings();
			be_roster->Launch(SCREEN_BLANKER_SIG, &settings);
			break;
		}
		case kAdd_btn:
			fFilePanel->Show();
			break;
		case kUpdatelist:
			PopulateScreenSaverList();
			break;
  	}
	BWindow::MessageReceived(msg);
}


bool 
ScreenSaverWin::QuitRequested() 
{
	UpdateStatus();
	fPrefs.SaveSettings();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}       


void 
ScreenSaverWin::UpdateStatus(void) 
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
	fPrefs.SetBlankTime(1000000LL* kTimeInUnits[fRunSlider->Value()]);
	fPrefs.SetOffTime(1000000LL* kTimeInUnits[fTurnOffSlider->Value()]);
	fPrefs.SetSuspendTime(1000000LL* kTimeInUnits[fTurnOffSlider->Value()]);
	fPrefs.SetStandbyTime(1000000LL* kTimeInUnits[fTurnOffSlider->Value()]);
	fPrefs.SetBlankCorner(fFadeNow->getDirection());
	fPrefs.SetNeverBlankCorner(fFadeNever->getDirection());
	fPrefs.SetLockEnable(fPasswordCheckbox->Value());
	fPrefs.SetPasswordTime(1000000LL* kTimeInUnits[fPasswordSlider->Value()]);
	int selection = fListView1->CurrentSelection();
	if (selection >= 0) {
		fPrefs.SetModuleName(((ScreenSaverItem *)(fListView1->ItemAt(selection)))->Text());
		if (strcmp(fPrefs.ModuleName(), "Blackness") == 0)
			fPrefs.SetModuleName("");
	}
// TODO - Tell the password window to update its stuff
	BMessage ssState;
	if ((fPreviewDisplay->ScreenSaver()) && (fPreviewDisplay->ScreenSaver()->SaveState(&ssState)==B_OK))
		fPrefs.SetState(fPrefs.ModuleName(), &ssState);
};


void 
ScreenSaverWin::SetupForm() 
{
	fFilePanel = new BFilePanel();

	BRect r = Bounds();
	BTab *tab1, *tab2;
	
// Create a background view
	BView *background = new BView(r,"background",B_FOLLOW_NONE,0);
	background->SetViewColor(216,216,216,0);
	AddChild(background);

// Add the tab view to the background
	r.top += 4;
	fTabView = new BTabView(r, "tab_view");
	fTabView->SetViewColor(216,216,216,0);
	r = fTabView->Bounds();
	r.InsetBy(0,4);
	r.bottom -= fTabView->TabHeight();

// Time to load the settings into a message and implement them...
	fPrefs.LoadSettings();

	tab2 = new BTab();
	fTab2 = new BView(r,"Fade",B_FOLLOW_NONE,0);
	tab2->SetLabel("Fade");

	tab1 = new CustomTab(this);
	fTab1 = new BView(r,"Modules",B_FOLLOW_NONE,0);
	tab1->SetLabel("Modules");

// Create the controls inside the tabs
	SetupTab2();
	SetupTab1();
	
	fTabView->AddTab(fTab2, tab2);
	fTabView->AddTab(fTab1, tab1);
	background->AddChild(fTabView);

// Create the password editing window
	fPwWin = new PasswordWindow(fPrefs);
	fPwMessenger = new BMessenger(NULL,fPwWin);
	fPwWin->Run();

	MoveTo(fPrefs.WindowFrame().left,fPrefs.WindowFrame().top);

	fEnableCheckbox->SetValue(fPrefs.TimeFlags());
	fRunSlider->SetValue(UnitsToSlider(fPrefs.BlankTime()));
	fTurnOffSlider->SetValue(UnitsToSlider(fPrefs.OffTime()));
	fFadeNow->setDirection(fPrefs.GetBlankCorner());
	fFadeNever->setDirection(fPrefs.GetNeverBlankCorner());
	fPasswordCheckbox->SetValue(fPrefs.LockEnable());
	fPasswordSlider->SetValue(UnitsToSlider(fPrefs.PasswordTime()));

	fTabView->Select(fPrefs.WindowTab());
	SelectCurrentModule();
	UpdateStatus();
} 


void
ScreenSaverWin::SelectCurrentModule()
{
	CALLED();
	
	int32 count = fListView1->CountItems();
	for (int32 i=0; i<count; i++) {
		ScreenSaverItem *item = dynamic_cast<ScreenSaverItem*>(fListView1->ItemAt(i));
		if ((strcmp(fPrefs.ModuleName(), item->Text()) == 0)
			|| (strcmp(fPrefs.ModuleName(),"") == 0 && strcmp(item->Text(), "Blackness") ==0)) {
			fListView1->Select(i);
			SaverSelected();
			fListView1->ScrollToSelection();
			break;
		}
	}
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
addScreenSaversToList (directory_which dir, BListView *list) 
{
	BPath path;
	find_directory(dir,&path);
	path.Append("Screen Savers",true);
  
	BDirectory ssDir(path.Path());
	BEntry thisSS;
	char thisName[B_FILE_NAME_LENGTH];

	while (B_OK==ssDir.GetNextEntry(&thisSS,true)) {
		thisSS.GetName(thisName);
		BString pathname = path.Path();
		pathname += "/";
		pathname += thisName;
		list->AddItem(new ScreenSaverItem(thisName, pathname.String())); 
	}
}


// sorting function for ScreenSaverItems
int 
ScreenSaverWin::CompareScreenSaverItems(const void* left, const void* right) 
{
	ScreenSaverItem* leftItem  = *(ScreenSaverItem **)left;
	ScreenSaverItem* rightItem = *(ScreenSaverItem **)right;
  
	return strcmp(leftItem->Text(),rightItem->Text());
}


void 
ScreenSaverWin::PopulateScreenSaverList(void) 
{
 	fListView1->DeselectAll(); 
	while (void *i=fListView1->RemoveItem((int32)0))
		delete ((ScreenSaverItem *)i);

	fListView1->AddItem(new ScreenSaverItem("Blackness", ""));
	addScreenSaversToList(B_BEOS_ADDONS_DIRECTORY, fListView1);
        addScreenSaversToList(B_USER_ADDONS_DIRECTORY, fListView1);

	fListView1->SortItems(CompareScreenSaverItems);
}


// Create the controls for the first tab
void 
ScreenSaverWin::SetupTab1() 
{
	{rgb_color clr = {216,216,216,255}; fTab1->SetViewColor(clr);}
	fTab1->AddChild( fModuleSettingsBox = new BBox(BRect(183,11,434,285),"ModuleSettingsBox",B_FOLLOW_NONE,B_WILL_DRAW));
	fModuleSettingsBox->SetLabel("Module settings");
	fModuleSettingsBox->SetBorder(B_FANCY_BORDER);

	fListView1 = new BListView(BRect(16,136,156,255),"ListView1",B_SINGLE_SELECTION_LIST);
	fTab1->AddChild(new BScrollView("scroll_list",fListView1,B_FOLLOW_NONE,0,false,true));
	commonLookAndFeel(fModuleSettingsBox,false,false);
	{rgb_color clr = {255,255,255,0}; fListView1->SetViewColor(clr);}
	fListView1->SetListType(B_SINGLE_SELECTION_LIST);

	// selection message for screensaver list
	fListView1->SetSelectionMessage( new BMessage( kSaver_sel ) );
  
	fTab1->AddChild( fTestButton = new BButton(BRect(13,260,88,277),"TestButton","Test", new BMessage (kTest_btn)));
	commonLookAndFeel(fTestButton,false,true);
	fTestButton->SetLabel("Test");

	fTab1->AddChild( fAddButton = new BButton(BRect(98,260,173,277),"AddButton","Add...", new BMessage (kAdd_btn)));
	commonLookAndFeel(fAddButton,false,true);
	fAddButton->SetLabel("Add...");

	fTab1->AddChild(fPreviewDisplay = new PreviewView(BRect(20,15,150,120),"preview",&fPrefs));
 	PopulateScreenSaverList(); 
} 


// Create the controls for the second tab
void 
ScreenSaverWin::SetupTab2() 
{
	font_height height;
	be_plain_font->GetHeight(&height);  
	int32 stringHeight = (int32)(height.ascent+height.descent);
	int32 sliderHeight = 30;
	int topEdge;
	{rgb_color clr = {216,216,216,255}; fTab2->SetViewColor(clr);}
	fTab2->AddChild( fEnableScreenSaverBox = new BBox(BRect(8,6,436,287),"EnableScreenSaverBox"));
	commonLookAndFeel(fEnableScreenSaverBox,false,false);

	fEnableCheckbox = new BCheckBox(BRect(0,0,90,stringHeight),"EnableCheckBox","Enable Screen Saver", new BMessage(kEnableScreenSaverChanged));
	fEnableScreenSaverBox->SetLabel(fEnableCheckbox);
	fEnableScreenSaverBox->SetBorder(B_FANCY_BORDER);

	// Run Module
	topEdge=32;
	fEnableScreenSaverBox->AddChild( fStringView1 = new BStringView(BRect(40,topEdge+2,130,topEdge+2+stringHeight),"StringView1","Run module"));
	commonLookAndFeel(fStringView1,false,false);
	fStringView1->SetText("Run module");
	fStringView1->SetAlignment(B_ALIGN_LEFT);

	fEnableScreenSaverBox->AddChild( fRunSlider = new BSlider(BRect(135,topEdge,420,topEdge+sliderHeight),"RunSlider","minutes", new BMessage(kRunSliderChanged), 0, 25));
	fRunSlider->SetModificationMessage(new BMessage(kRunSliderChanged));
	commonLookAndFeel(fRunSlider,true,true);
	float w,h;
	fRunSlider->GetPreferredSize(&w,&h);
	sliderHeight= (int32)h - 6;

	// Turn Off
	topEdge+=sliderHeight;
	fEnableScreenSaverBox->AddChild( fTurnOffScreenCheckBox = new BCheckBox(BRect(20,topEdge-2,127,topEdge-2+stringHeight),"TurnOffScreenCheckBox","Turn off screen",  new BMessage (kTab2_chg)));
	commonLookAndFeel(fTurnOffScreenCheckBox,false,true);
	fTurnOffScreenCheckBox->SetLabel("Turn off screen");
	fTurnOffScreenCheckBox->SetResizingMode(B_FOLLOW_NONE);

	fEnableScreenSaverBox->AddChild( fTurnOffSlider = new BSlider(BRect(135,topEdge,420,topEdge+sliderHeight),"TurnOffSlider","", new BMessage(kTurnOffSliderChanged), 0, 25));
	fTurnOffSlider->SetModificationMessage(new BMessage(kTurnOffSliderChanged));
	commonLookAndFeel(fTurnOffSlider,true,true);

	// Password
	topEdge+=sliderHeight;
	fEnableScreenSaverBox->AddChild( fPasswordCheckbox = new BCheckBox(BRect(20,topEdge-2,127,topEdge-2+stringHeight),"PasswordCheckbox","Password lock",  new BMessage (kPasswordCheckbox)));
	commonLookAndFeel(fPasswordCheckbox,false,true);
	fPasswordCheckbox->SetLabel("Password lock");

	fEnableScreenSaverBox->AddChild( fPasswordSlider = new BSlider(BRect(135,topEdge,420,topEdge+sliderHeight),"PasswordSlider","", new BMessage(kPasswordSliderChanged), 0, 25));
	fPasswordSlider->SetModificationMessage(new BMessage(kPasswordSliderChanged));
	commonLookAndFeel(fPasswordSlider,true,true);

	topEdge+=sliderHeight - 6;
	fEnableScreenSaverBox->AddChild( fPasswordButton = new BButton(BRect(335,topEdge,410,topEdge+24),"PasswordButton","Password...",  new BMessage (kPwbutton)));
	commonLookAndFeel(fPasswordButton,false,true);
	fPasswordButton->SetLabel("Password...");

	// Bottom

	fEnableScreenSaverBox->AddChild(fFadeNow=new MouseAreaView(BRect(20,205,80,260),"fadeNow"));
	fEnableScreenSaverBox->AddChild(fFadeNever=new MouseAreaView(BRect(220,205,280,260),"fadeNever"));

	fEnableScreenSaverBox->AddChild( fFadeNowString = new BStringView(BRect(90,215,193,230),"FadeNowString","Fade now when"));
	commonLookAndFeel(fFadeNowString,false,false);
	fFadeNowString->SetText("Fade now when");
	fFadeNowString->SetAlignment(B_ALIGN_LEFT);

	fEnableScreenSaverBox->AddChild( fFadeNowString2 = new BStringView(BRect(90,233,193,248),"FadeNowString2","mouse is here"));
	commonLookAndFeel(fFadeNowString2,false,false);
	fFadeNowString2->SetText("mouse is here");
	fFadeNowString2->SetAlignment(B_ALIGN_LEFT);

	fEnableScreenSaverBox->AddChild( fDontFadeString = new BStringView(BRect(290,215,387,230),"DontFadeString","Don't fade when"));
	commonLookAndFeel(fDontFadeString,false,false);
	fDontFadeString->SetText("Don't fade when");
	fDontFadeString->SetAlignment(B_ALIGN_LEFT);

	fEnableScreenSaverBox->AddChild( fDontFadeString2 = new BStringView(BRect(290,233,387,248),"DontFadeString2","mouse is here"));
	commonLookAndFeel(fDontFadeString2,false,false);
	fDontFadeString2->SetText("mouse is here");
	fDontFadeString2->SetAlignment(B_ALIGN_LEFT);
}
