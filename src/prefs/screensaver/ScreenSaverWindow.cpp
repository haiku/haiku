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
#include <stdio.h>

void drawPositionalMonitor(BView *view,BRect areaToDrawIn,int state);
BView *drawSampleMonitor(BView *view, BRect area);

struct SSListItem {
  BString fileName;
  BString displayName;	
};

void ScreenSaverWin::MessageReceived(BMessage *msg) {
	SSListItem* listItem;
	switch(msg->what) {
		case PWBUTTON: 
			pwMessenger->SendMessage(SHOW);
      		break;
		case B_QUIT_REQUESTED:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		case SAVER_SEL:
			if (previewDisplay->ScreenSaver())
				previewDisplay->ScreenSaver()->StopConfig();
   			listItem = reinterpret_cast<SSListItem*>(AddonList->ItemAt(ListView1->CurrentSelection()));
			BString settingsMsgName(listItem->fileName);
			previewDisplay->SetScreenSaver(settingsMsgName);
			if (settingsArea) {
				ModuleSettingsBox->RemoveChild(settingsArea);
				delete settingsArea;
				}
			BRect bnds=ModuleSettingsBox->Bounds();
			bnds.InsetBy(5,10);
			settingsArea=new BView(bnds,"settingsArea",B_FOLLOW_NONE,B_WILL_DRAW);
			settingsArea->SetViewColor(216,216,216);
			ModuleSettingsBox->AddChild(settingsArea);
			
			if (previewDisplay->ScreenSaver())
				previewDisplay->ScreenSaver()->StartConfig(settingsArea);
    	break;
  	}
	updateStatus(); // This could get called sometimes when it doesn't need to. Shouldn't hurt
	BWindow::MessageReceived(msg);
}

bool ScreenSaverWin::QuitRequested() {
	updateStatus();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}       

void ScreenSaverWin::updateStatus(void) {
	DisableUpdates();
	// Policy - enable and disable controls as per checkboxes, etc
	PasswordCheckbox->SetEnabled(EnableCheckbox->Value());
	TurnOffScreenCheckBox->SetEnabled(EnableCheckbox->Value());
	RunSlider->SetEnabled(EnableCheckbox->Value());
	TurnOffSlider->SetEnabled(EnableCheckbox->Value() && TurnOffScreenCheckBox->Value());
	TurnOffSlider->SetEnabled(false); // This never seems to turn on in the R5 version
	TurnOffScreenCheckBox->SetEnabled(false);
	PasswordSlider->SetEnabled(EnableCheckbox->Value() && PasswordCheckbox->Value());
	PasswordButton->SetEnabled(EnableCheckbox->Value() && PasswordCheckbox->Value());
	// Set the labels for the sliders
	RunSlider->SetLabel(times[RunSlider->Value()]);
	TurnOffSlider->SetLabel(times[TurnOffSlider->Value()]);
	PasswordSlider->SetLabel(times[PasswordSlider->Value()]);
	EnableUpdates();
	// Update the saved preferences
	prefs.SetWindowFrame(Frame());	
	prefs.SetWindowTab(tabView->Selection());
	prefs.SetTimeFlags(EnableCheckbox->Value());
	prefs.SetBlankTime(timeInSeconds[RunSlider->Value()]);
	prefs.SetOffTime(timeInSeconds[TurnOffSlider->Value()]);
	prefs.SetSuspendTime(timeInSeconds[TurnOffSlider->Value()]);
	prefs.SetStandbyTime(timeInSeconds[TurnOffSlider->Value()]);
	prefs.SetBlankCorner(fadeNow->getDirection());
	prefs.SetNeverBlankCorner(fadeNever->getDirection());
	prefs.SetLockEnable(PasswordCheckbox->Value());
	prefs.SetPasswordTime(timeInSeconds[PasswordSlider->Value()]);
	int selection=ListView1->CurrentSelection(0);
	//const BStringItem **ptr = (const BStringItem **)(ListView1->Items());
	if (selection>=0)
		prefs.SetModuleName(((BStringItem *)(ListView1->ItemAt(selection)))->Text());
// TODO - Tell the password window to update its stuff
	prefs.SaveSettings();
};

void ScreenSaverWin::SetupForm(void) {
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
	tabView = new BTabView(r, "tab_view");
	tabView->SetViewColor(216,216,216,0);
	r = tabView->Bounds();
	r.InsetBy(0,4);
	r.bottom -= tabView->TabHeight();

	tab = new BTab();
	tabView->AddTab(tab2=new BView(r,"Fade",B_FOLLOW_NONE,0), tab);
	tab->SetLabel("Fade");

	tab = new BTab();
	tabView->AddTab(tab1=new BView(r,"Modules",B_FOLLOW_NONE,0), tab);
	tab->SetLabel("Modules");
	background->AddChild(tabView);

// Create the controls inside the tabs
	setupTab2();
	setupTab1();

// Create the password editing window
	pwWin=new pwWindow;
	pwMessenger=new BMessenger (NULL,pwWin);
	pwWin->Run();
// Time to load the settings into a message and implement them...
	prefs.LoadSettings();
	prefs.WindowFrame().PrintToStream();
	MoveTo(prefs.WindowFrame().left,prefs.WindowFrame().top);
	ResizeTo(prefs.WindowFrame().right-prefs.WindowFrame().left,prefs.WindowFrame().bottom-prefs.WindowFrame().top);
	tabView->Select(prefs.WindowTab());
	EnableCheckbox->SetValue(prefs.TimeFlags());
	RunSlider->SetValue(secondsToSlider(prefs.BlankTime()));
	TurnOffSlider->SetValue(secondsToSlider(prefs.OffTime()));
	fadeNow->setDirection(prefs.GetBlankCorner());
	fadeNever->setDirection(prefs.GetNeverBlankCorner());
	PasswordCheckbox->SetValue(prefs.LockEnable());
	PasswordSlider->SetValue(secondsToSlider(prefs.PasswordTime()));
	const BStringItem **ptr = (const BStringItem **)(ListView1->Items());
	long count=ListView1->CountItems();
	if (prefs.Password() && ptr) 	
		for ( long i = 0; i < count; i++ ) {
			if (BString(prefs.Password())==((*ptr++)->Text()))
				ListView1->Select(count=i); // Clever bit here - intentional assignment.
			}
	updateStatus();
} 

// Set the common Look and Feel stuff for a given control
void commonLookAndFeel(BView *widget,bool isSlider,bool isControl) {
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
void addScreenSaversToList (directory_which dir, BList *list) {
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
int compareSSListItems(const void* left, const void* right) {
  SSListItem* leftItem  = *(SSListItem **)left;
  SSListItem* rightItem = *(SSListItem **)right;
  
  return leftItem->displayName.Compare(rightItem->displayName);
}

// Add the strings in the BList to a BListView
void displayScreenSaversList(BList* list, BListView* view) {
  list->SortItems(compareSSListItems);
  
  int numItems = list->CountItems();
  for( int i = 0; i < numItems; ++i ) {
    SSListItem* item = (SSListItem*)(list->ItemAt(i));
    view->AddItem( new BStringItem(item->displayName.String()) );
  }
}

// Create the controls for the first tab
void ScreenSaverWin::setupTab1(void) {
  int columns[4]={15,150,180,430};
  int rows[6]={15,125,135,255,263,276};

  {rgb_color clr = {216,216,216,255}; tab1->SetViewColor(clr);}
  tab1->AddChild( ModuleSettingsBox = new BBox(BRect(columns[2],rows[0],columns[3],rows[5]),"ModuleSettingsBox",B_FOLLOW_NONE,B_WILL_DRAW));
  ModuleSettingsBox->SetLabel("Module settings");
  ModuleSettingsBox->SetBorder(B_FANCY_BORDER);

  ListView1 = new BListView(BRect(columns[0],rows[2],columns[1]+3,rows[3]),"ListView1",B_SINGLE_SELECTION_LIST);
  tab1->AddChild(new BScrollView("scroll_list",ListView1,B_FOLLOW_NONE,0,false,true));
  commonLookAndFeel(ModuleSettingsBox,false,false);
  {rgb_color clr = {255,255,255,0}; ListView1->SetViewColor(clr);}
  ListView1->SetListType(B_SINGLE_SELECTION_LIST);

  // selection message for screensaver list
  ListView1->SetSelectionMessage( new BMessage( SAVER_SEL ) );
  
  tab1->AddChild( TestButton = new BButton(BRect(columns[0],rows[4],91,rows[5]),"TestButton","Test", new BMessage (TAB1_CHG)));
  commonLookAndFeel(TestButton,false,true);
  TestButton->SetLabel("Test");

  tab1->AddChild( AddButton = new BButton(BRect(97,rows[4],columns[2]-10,rows[5]),"AddButton","Add...", new BMessage (TAB1_CHG)));
  commonLookAndFeel(AddButton,false,true);
  AddButton->SetLabel("Add...");

  tab1->AddChild(previewDisplay = new PreviewView(BRect(columns[0]+5,rows[0],columns[1],rows[1]),"preview",&prefs));
  
  AddonList = new BList;
  
  addScreenSaversToList( B_BEOS_ADDONS_DIRECTORY, AddonList );
  addScreenSaversToList( B_USER_ADDONS_DIRECTORY, AddonList );
  
  displayScreenSaversList( AddonList, ListView1 );
} 


// Create the controls for the second tab
void ScreenSaverWin::setupTab2(void) {
  font_height stdFontHt;
  be_plain_font->GetHeight(&stdFontHt);  
  int stringHeight=(int)(stdFontHt.ascent+stdFontHt.descent),sliderHeight=30;
  int topEdge;
  {rgb_color clr = {216,216,216,255}; tab2->SetViewColor(clr);}
  tab2->AddChild( EnableScreenSaverBox = new BBox(BRect(11,13,437,280),"EnableScreenSaverBox"));
  commonLookAndFeel(EnableScreenSaverBox,false,false);

  EnableCheckbox = new BCheckBox(BRect(0,0,90,stringHeight),"EnableCheckBox","Enable Screen Saver", new BMessage (TAB2_CHG));
  EnableScreenSaverBox->SetLabel(EnableCheckbox);
  EnableScreenSaverBox->SetBorder(B_FANCY_BORDER);

  // Run Module
  topEdge=26;
  EnableScreenSaverBox->AddChild( StringView1 = new BStringView(BRect(21,topEdge,101,topEdge+stringHeight),"StringView1","Run module"));
  commonLookAndFeel(StringView1,false,false);
  StringView1->SetText("Run module");
  StringView1->SetAlignment(B_ALIGN_LEFT);

  EnableScreenSaverBox->AddChild( RunSlider = new BSlider(BRect(132,topEdge,415,topEdge+sliderHeight),"RunSlider","minutes", new BMessage(TAB2_CHG), 0, 25));
  RunSlider->SetModificationMessage(new BMessage(TAB2_CHG));
  commonLookAndFeel(RunSlider,true,true);
  float w,h;
  RunSlider->GetPreferredSize(&w,&h);
  sliderHeight=(int)h;

  // Turn Off
  topEdge+=sliderHeight;
  EnableScreenSaverBox->AddChild( TurnOffScreenCheckBox = new BCheckBox(BRect(9,topEdge,107,topEdge+stringHeight),"TurnOffScreenCheckBox","Turn off screen",  new BMessage (TAB2_CHG)));
  commonLookAndFeel(TurnOffScreenCheckBox,false,true);
  TurnOffScreenCheckBox->SetLabel("Turn off screen");
  TurnOffScreenCheckBox->SetResizingMode(B_FOLLOW_NONE);

  EnableScreenSaverBox->AddChild( TurnOffSlider = new BSlider(BRect(132,topEdge,415,topEdge+sliderHeight),"TurnOffSlider","", new BMessage(TAB2_CHG), 0, 25));
  TurnOffSlider->SetModificationMessage(new BMessage(TAB2_CHG));
  commonLookAndFeel(TurnOffSlider,true,true);

  // Password
  topEdge+=sliderHeight;
  EnableScreenSaverBox->AddChild( PasswordCheckbox = new BCheckBox(BRect(9,topEdge,108,topEdge+stringHeight),"PasswordCheckbox","Password lock",  new BMessage (TAB2_CHG)));
  commonLookAndFeel(PasswordCheckbox,false,true);
  PasswordCheckbox->SetLabel("Password lock");

  EnableScreenSaverBox->AddChild( PasswordSlider = new BSlider(BRect(132,topEdge,415,topEdge+sliderHeight),"PasswordSlider","", new BMessage(TAB2_CHG), 0, 25));
  PasswordSlider->SetModificationMessage(new BMessage(TAB2_CHG));
  commonLookAndFeel(PasswordSlider,true,true);

  topEdge+=sliderHeight;
  EnableScreenSaverBox->AddChild( PasswordButton = new BButton(BRect(331,topEdge,405,topEdge+25),"PasswordButton","Password...",  new BMessage (PWBUTTON)));
  commonLookAndFeel(PasswordButton,false,true);
  PasswordButton->SetLabel("Password...");

	// Bottom

  EnableScreenSaverBox->AddChild(fadeNow=new MouseAreaView(BRect(20,205,80,260),"fadeNow"));
  EnableScreenSaverBox->AddChild(fadeNever=new MouseAreaView(BRect(220,205,280,260),"fadeNever"));

  EnableScreenSaverBox->AddChild( FadeNowString = new BStringView(BRect(85,210,188,222),"FadeNowString","Fade now when"));
  commonLookAndFeel(FadeNowString,false,false);
  FadeNowString->SetText("Fade now when");
  FadeNowString->SetAlignment(B_ALIGN_LEFT);

  EnableScreenSaverBox->AddChild( FadeNowString2 = new BStringView(BRect(85,225,188,237),"FadeNowString2","mouse is here"));
  commonLookAndFeel(FadeNowString2,false,false);
  FadeNowString2->SetText("mouse is here");
  FadeNowString2->SetAlignment(B_ALIGN_LEFT);

  EnableScreenSaverBox->AddChild( DontFadeString = new BStringView(BRect(285,210,382,222),"DontFadeString","Don't fade when"));
  commonLookAndFeel(DontFadeString,false,false);
  DontFadeString->SetText("Don't fade when");
  DontFadeString->SetAlignment(B_ALIGN_LEFT);

  EnableScreenSaverBox->AddChild( DontFadeString2 = new BStringView(BRect(285,225,382,237),"DontFadeString2","mouse is here"));
  commonLookAndFeel(DontFadeString2,false,false);
  DontFadeString2->SetText("mouse is here");
  DontFadeString2->SetAlignment(B_ALIGN_LEFT);
}

