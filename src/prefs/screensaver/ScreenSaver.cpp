#include "ScreenSaver.h"
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
#include <iostream>
#include "MouseAreaView.h"
#include "PreviewView.h"

void drawPositionalMonitor(BView *view,BRect areaToDrawIn,int state);
BView *drawSampleMonitor(BView *view, BRect area);
  int columns[4]={15,175,195,430};
  int rows[6]={10,150,155,255,270,290};

struct SSListItem
{
  BString fileName;
  BString displayName;	
};

void ScreenSaver::MessageReceived(BMessage *msg)
{
  switch(msg->what)
  {
	case TAB1_CHG:
		updateStatus();
      	BWindow::MessageReceived(msg);
      	break;
	case TAB2_CHG:
		updateStatus();
      	BWindow::MessageReceived(msg);
      	break;
	case PWBUTTON:
		{
		updateStatus();
		pwMessenger->SendMessage(SHOW);
      	BWindow::MessageReceived(msg);
		}
      	break;
	case B_QUIT_REQUESTED:
		be_app->PostMessage(B_QUIT_REQUESTED);
      	BWindow::MessageReceived(msg);
      	break;
    case SAVER_SEL:
   		updateStatus();
		SelectedAddonFileName = reinterpret_cast<SSListItem*>(AddonList->ItemAt(ListView1->CurrentSelection()))->fileName;
		previewDisplay->LoadNewAddon(SelectedAddonFileName.String());
    	BWindow::MessageReceived(msg);
    	break;
    default:
      	BWindow::MessageReceived(msg);
      	break;
  }
}

void ScreenSaver::loadSettings(BMessage *msg)
{
	BRect frame;
	if (B_OK == msg->FindRect("windowframe",&frame)) {
		MoveTo(frame.left,frame.top);
		ResizeTo(frame.right-frame.left,frame.bottom-frame.top);
	}

	int32 value;
	msg->FindInt32("windowtab",&value);
	tabView->Select(value);
	msg->FindInt32("timeflags",&value);
	EnableCheckbox->SetValue(value);
	msg->FindInt32("timefade",&value);
	value=secondsToSlider(value);
	if (value>=0) RunSlider->SetValue(value);
	msg->FindInt32("timestandby",&value);
	value=secondsToSlider(value);
	if (value>=0) TurnOffSlider->SetValue(value);

	msg->FindInt32("cornernow",&value);
	fadeNow->setDirection(value);
	msg->FindInt32("cornernever",&value);
	fadeNever->setDirection(value);

	bool enable;
	msg->FindBool("lockenable",&enable);
	PasswordCheckbox->SetValue(enable);
	msg->FindInt32("lockdelay",&value);
	value=secondsToSlider(value);
	if (value>=0) PasswordSlider->SetValue(value);

	BString name;
	msg->FindString("modulename",&name);
	const BStringItem **ptr = (const BStringItem **)(ListView1->Items());
	long count=ListView1->CountItems();
	for ( long i = 0; i < count; i++ )
		{
		if (name==((*ptr)->Text()))
			ListView1->Select(count=i); // Clever bit here - intentional assignment.
		else 
			*ptr++;
		}
	msg->what=UTILIZE;	
	pwMessenger->SendMessage(msg);
	// Pass to the module for its parameters...
}

void ScreenSaver::saveSettings(void)
{
	BMessage msg;
	msg.AddRect("windowframe",Frame());
	msg.AddInt32("windowtab",tabView->Selection());
	msg.AddInt32("timeflags",EnableCheckbox->Value());
	msg.AddInt32("timefade", timeInSeconds[RunSlider->Value()]);
	msg.AddInt32("timestandby", timeInSeconds[TurnOffSlider->Value()]);
	msg.AddInt32("timesuspend", timeInSeconds[TurnOffSlider->Value()]);
	msg.AddInt32("timeoff", timeInSeconds[TurnOffSlider->Value()]);
	msg.AddInt32("cornernow", fadeNow->getDirection());
	msg.AddInt32("cornernever", fadeNever->getDirection());
	msg.AddBool("lockenable",PasswordCheckbox->Value());
	msg.AddInt32("lockdelay", timeInSeconds[PasswordSlider->Value()]);
	int selection=ListView1->CurrentSelection(0);
	if (selection>=0)
		msg.AddString("modulename", ((BStringItem *)(ListView1->ItemAt(selection)))->Text()); 
	msg.what=POPULATE;
	BMessage newMsg;
	pwMessenger->SendMessage(&msg,&newMsg);
	// Pass this message to the loaded screen saver so that it can add its own preferences
		/* > B_MESSAGE_TYPE        "modulesettings_SuperString"
			>  | What=B_OK
			>  | B_BOOL_TYPE           "fade"                   1
			> B_STRING_TYPE         "modulename"             "Lissart"
		*/
  BPath path;
  find_directory(B_USER_SETTINGS_DIRECTORY,&path);
  path.Append("OBOS_Screen_Saver",true);
  BFile file(path.Path(),B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
  newMsg.Flatten(&file);
}

bool ScreenSaver::QuitRequested()
{
	updateStatus();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}       

void ScreenSaver::updateStatus(void)
{
  DisableUpdates();
  PasswordCheckbox->SetEnabled(EnableCheckbox->Value());
  TurnOffScreenCheckBox->SetEnabled(EnableCheckbox->Value());
  RunSlider->SetEnabled(EnableCheckbox->Value());
  TurnOffSlider->SetEnabled(EnableCheckbox->Value() && TurnOffScreenCheckBox->Value());
  TurnOffSlider->SetEnabled(false);
  TurnOffScreenCheckBox->SetEnabled(false);
  PasswordSlider->SetEnabled(EnableCheckbox->Value() && PasswordCheckbox->Value());
  PasswordButton->SetEnabled(EnableCheckbox->Value() && PasswordCheckbox->Value());
  RunSlider->SetLabel(times[RunSlider->Value()]);
  TurnOffSlider->SetLabel(times[TurnOffSlider->Value()]);
  PasswordSlider->SetLabel(times[PasswordSlider->Value()]);
  EnableUpdates();
  saveSettings();
};

void ScreenSaver::SetupForm(void)
{
	BRect r;
	BView *background;
	BTab *tab;
	r = Bounds();

	background=new BView(r,"background",B_FOLLOW_NONE,0);
	background->SetViewColor(216,216,216,0);
	AddChild(background);

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
	setupTab2();
	setupTab1();
	pwWin=new pwWindow;
	pwMessenger=new BMessenger (NULL,pwWin);
	
	//chng 5/31/02 adg
	// replace this:
	//pwWin->Show();
	//pwWin->Hide();
	// with this:
	pwWin->Run();
	//end chng
	
	// Time to load the settings into a message and implement them...
  	BPath path;
  	find_directory(B_USER_SETTINGS_DIRECTORY,&path);
  	path.Append("OBOS_Screen_Saver",true);
  	BFile file(path.Path(),B_READ_ONLY);
	BMessage msg;
  	msg.Unflatten(&file);
	loadSettings (&msg);
	updateStatus();
}

void commonLookAndFeel(BView *widget,bool isSlider,bool isControl)
	{
  	{rgb_color clr = {216,216,216,255}; widget->SetViewColor(clr);}
	if (isSlider)
		{
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
	if (isControl)
		{
		BControl *wid=dynamic_cast<BControl *>(widget);
		wid->SetEnabled(true);
		}
	}

//*******
void addScreenSaversToList (directory_which dir, BList *list)
{
  BPath path;
  find_directory(dir,&path);
  path.Append("Screen Savers",true);
  
  const char* pathName = path.Path();
  
  BDirectory ssDir(pathName);
  BEntry thisSS;
  char thisName[B_FILE_NAME_LENGTH];

  while (B_OK==ssDir.GetNextEntry(&thisSS,true))
  	{
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
int compareSSListItems(const void* left, const void* right)
{
  SSListItem* leftItem  = *(SSListItem **)left;
  SSListItem* rightItem = *(SSListItem **)right;
  
  return leftItem->displayName.Compare(rightItem->displayName);
}

void displayScreenSaversList(BList* list, BListView* view)
{
  list->SortItems(compareSSListItems);
  
  int numItems = list->CountItems();
  for( int i = 0; i < numItems; ++i )
  {
    SSListItem* item = (SSListItem*)(list->ItemAt(i));
    view->AddItem( new BStringItem(item->displayName.String()) );
  }
}

void ScreenSaver::setupTab1(void)
{
  {rgb_color clr = {216,216,216,255}; tab1->SetViewColor(clr);}
  tab1->AddChild( ModuleSettingsBox = new BBox(BRect(columns[2],rows[0],columns[3],rows[5]),"ModuleSettingsBox"));
  commonLookAndFeel(ModuleSettingsBox,false,false);
  ModuleSettingsBox->SetLabel("Module settings");
  ModuleSettingsBox->SetBorder(B_FANCY_BORDER);

  ListView1 = new BListView(BRect(columns[0],rows[2],columns[1],rows[3]),"ListView1",B_SINGLE_SELECTION_LIST);
  tab1->AddChild(new BScrollView("scroll_list",ListView1,B_FOLLOW_NONE,0,false,true));
  commonLookAndFeel(ModuleSettingsBox,false,false);
  {rgb_color clr = {255,255,255,0}; ListView1->SetViewColor(clr);}
  ListView1->SetListType(B_SINGLE_SELECTION_LIST);

  // selection message for screensaver list
  ListView1->SetSelectionMessage( new BMessage( SAVER_SEL ) );
  
  tab1->AddChild( TestButton = new BButton(BRect(columns[0],rows[4],94,rows[5]),"TestButton","Test", new BMessage (TAB1_CHG)));
  commonLookAndFeel(TestButton,false,true);
  TestButton->SetLabel("Test");

  tab1->AddChild( AddButton = new BButton(BRect(97,rows[4],columns[1],rows[5]),"AddButton","Add...", new BMessage (TAB1_CHG)));
  commonLookAndFeel(AddButton,false,true);
  AddButton->SetLabel("Add...");

  tab1->AddChild(previewDisplay = new PreviewView(BRect(columns[0],rows[0],columns[1],rows[1]),"preview"));
  // -----------------------------------------------------------------------------------------
  // Populate the listview with the screensavers that exist.

  previewDisplay->SetSettingsBoxPtr( ModuleSettingsBox );

  AddonList = new BList;
  
  addScreenSaversToList( B_BEOS_ADDONS_DIRECTORY, AddonList );
  addScreenSaversToList( B_USER_ADDONS_DIRECTORY, AddonList );
  
  displayScreenSaversList( AddonList, ListView1 );
} //end setupTab1()


void ScreenSaver::setupTab2(void)
{
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

