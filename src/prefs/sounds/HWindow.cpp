#include "HWindow.h"
#include "HEventList.h"
#include "HEventItem.h"
#include "HTypeList.h"
#include "BetterScrollView.h"
#include "RectUtils.h"
#include "TextEntryAlert.h"

#include <ClassInfo.h>
#include <MediaFiles.h>
#include <MenuBar.h>
#include <Box.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Button.h>
#include <Beep.h>
#include <Path.h>
#include <Application.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Alert.h>
#include <Roster.h>
#include <fs_attr.h>
#include <stdio.h>
#include <Sound.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HWindow::HWindow(BRect rect ,const char* name)
	:_inherited(rect,name,B_TITLED_WINDOW,0)
	,fFilePanel(NULL)
	,fPlayer(NULL)
{
	InitMenu();
	InitGUI();
	float min_width,min_height,max_width,max_height;
	GetSizeLimits(&min_width,&max_width,&min_height,&max_height);
	min_width = 320;
	min_height = 150;
	SetSizeLimits(min_width,max_width,min_height,max_height);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HWindow::~HWindow()
{
	delete fFilePanel;
	delete fPlayer;
}

/***********************************************************
 * Initialize GUIs.
 ***********************************************************/
void
HWindow::InitGUI()
{
	BRect rect = Bounds();
	rect.top = KeyMenuBar()->Bounds().Height()+1;

	/*fTypeList = new HTypeList(rect);
	BScrollView *scrollView = new BScrollView(""
									,fTypeList
									,B_FOLLOW_LEFT|B_FOLLOW_TOP_BOTTOM
									,0
									,false
									,true);
	AddChild(scrollView);
	*/
	//rect.left = rect.right + B_V_SCROLL_BAR_WIDTH+3;
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	rect.bottom = Bounds().bottom - B_H_SCROLL_BAR_HEIGHT - 80;
	BetterScrollView *betterScrollView;
	fEventList = new HEventList(rect,(CLVContainerView**)&betterScrollView);
	fEventList->SetType(BMediaFiles::B_SOUNDS);
	AddChild(betterScrollView);
	
	app_info info; 
    be_app->GetAppInfo(&info); 
   	BEntry entry(&info.ref); 
   	float width;
	BFile appfile(&entry,B_WRITE_ONLY);
	attr_info ainfo;
	
	if(appfile.GetAttrInfo("event_width",&ainfo) == B_OK)
	{
		appfile.ReadAttr("event_width",B_FLOAT_TYPE,0,&width,sizeof(float));
		CLVColumn *col = fEventList->ColumnAt(1);
		if(col)
			col->SetWidth(width);
	}
	
	if(appfile.GetAttrInfo("sound_width",&ainfo) == B_OK)
	{
		appfile.ReadAttr("sound_width",B_FLOAT_TYPE,0,&width,sizeof(float));
		CLVColumn *col = fEventList->ColumnAt(2);
		if(col)
			col->SetWidth(width);
	}
	
	rect.top = rect.bottom +1;
	rect.bottom = Bounds().bottom;
	rect.right = Bounds().right;
	BView *view = new BView(rect,"",B_FOLLOW_LEFT_RIGHT|B_FOLLOW_BOTTOM,B_WILL_DRAW|B_PULSE_NEEDED);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);
	rect = view->Bounds();
	rect.top += 10;
	rect.bottom -= 10;
	rect.left += 10;
	rect.right -= 10;
	BBox *box = new BBox(rect,"",B_FOLLOW_ALL);
	view->AddChild(box);
	rect = box->Bounds();
	rect.top += 10;
	rect.left += 10;
	rect.right -= 10;
	rect.bottom = rect.top + 20;
	BMenu *menu = new BMenu("file");
	menu->SetRadioMode(true);
	menu->SetLabelFromMarked(true);
	menu->AddSeparatorItem();
	
	menu->AddItem(new BMenuItem("<none>",new BMessage(M_NONE_MESSAGE)));
	menu->AddItem(new BMenuItem("Other…",new BMessage(M_OTHER_MESSAGE)));
	BMenuField *menuField = new BMenuField(rect
										,"filemenu"
										,"Sound File:"
										,menu
										,B_FOLLOW_TOP|B_FOLLOW_LEFT);
	menuField->SetDivider(menuField->StringWidth("Media Files:")+3);
	box->AddChild(menuField);
	rect.OffsetBy(0,25);
	rect.left = rect.right - 80;
	BButton *button = new BButton(rect
						,"play"
						,"Play"
						,new BMessage(M_PLAY_MESSAGE)
						,B_FOLLOW_RIGHT|B_FOLLOW_TOP);
	box->AddChild(button);
	
	rect.OffsetBy(-90,0);
	button = new BButton(rect
								,"stop"
								,"Stop"
								,new BMessage(M_STOP_MESSAGE)
								,B_FOLLOW_RIGHT|B_FOLLOW_TOP);
	box->AddChild(button);
	
	rect.OffsetBy(-110,0);
	button = new BButton(rect
								,"setsystem"
								,"As System event"
								,NULL
								,B_FOLLOW_RIGHT|B_FOLLOW_TOP);
	button->ResizeToPreferred();
	box->AddChild(button);

	// setup file menu
	SetupMenuField();
	menu->FindItem("<none>")->SetMarked(true);
}

/***********************************************************
 * Initalize Menus
 ***********************************************************/
void
HWindow::InitMenu()
{
	BMenuBar *menuBar = new BMenuBar(Bounds(),"MENUBAR");
	BMenu *menu;
	BMenuItem *item;
	
	menu = new BMenu("File");
	item = new BMenuItem("Add New Event…",new BMessage(M_ADD_EVENT),'A');
	item->SetTarget(this);
	menu->AddItem(item);
	
	item = new BMenuItem("Remove Event",new BMessage(M_REMOVE_EVENT),'R');
	item->SetTarget(this);
	menu->AddItem(item);
	 
	menu->AddSeparatorItem();
	item = new BMenuItem("About openSounds…",new BMessage(B_ABOUT_REQUESTED),0,0);
	item->SetTarget(be_app);
	menu->AddItem(item);
	menu->AddSeparatorItem();
	item = new BMenuItem("Quit",new BMessage(B_QUIT_REQUESTED),'Q',0);
	menu->AddItem(item);
	menuBar->AddItem(menu);
	this->AddChild(menuBar);
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HWindow::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case M_OPEN_WITH:
		{
			entry_ref app_ref;
			if(message->FindRef("refs",&app_ref) == B_OK)
			{
				BMessage msg(B_REFS_RECEIVED);
				int32 sel = fEventList->CurrentSelection();
				if(sel < 0)
					break;
				HEventItem *item = cast_as(fEventList->ItemAt(sel),HEventItem);
				const char* path = item->Path();
				entry_ref ref;
				::get_ref_for_path(path,&ref);
				msg.AddRef("refs",&ref);
				be_roster->Launch(&app_ref,&msg);
			}
			break;
		}
	case M_REMOVE_EVENT:
		{
			RemoveEvent();
			break;
		}
	case M_ADD_EVENT:
		{
		char new_Event[20];
		(new BAlert("Add an event","Note that to use this event, applications should call him by the function:\n status_t system_beep(const char *eventName);\n declared on be/support/Beep.h","OK",NULL,NULL,
						B_WIDTH_AS_USUAL,B_WARNING_ALERT))->Go();		
		if (((new TextEntryAlert("Add an event ...","Type name: ", "new event name",
								 "Create","Cancel",true,120,1,B_WIDTH_AS_USUAL,false,
								 NULL,B_FLOATING_WINDOW_LOOK)
			 )->Go(new_Event,20))==0){
				add_system_beep_event(new_Event);
				
				BMediaFiles mfiles;
				mfiles.RewindRefs("Sound");				
				entry_ref ref;
		        mfiles.GetRefFor("Sound",new_Event,&ref);
				fEventList->AddEvent(new HEventItem(new_Event,BPath(&ref).Path()));
		}			
			break;
		}
	case M_OTHER_MESSAGE:
		{
			if(!fFilePanel)
				fFilePanel = new BFilePanel();
			fFilePanel->SetTarget(this);
			fFilePanel->Show();
			break;
		}
	case B_REFS_RECEIVED:
		{
			entry_ref ref;
			int32 sel = fEventList->CurrentSelection();
			if(message->FindRef("refs",&ref) == B_OK && sel >= 0)
			{
				BMenuField *menufield = cast_as(FindView("filemenu"),BMenuField);
				BMenu *menu = menufield->Menu();
				// check audio file
				BNode node(&ref);
				BNodeInfo ninfo(&node);
				char type[B_MIME_TYPE_LENGTH+1];
				ninfo.GetType(type);
				BMimeType mtype(type);
				BMimeType superType;
				mtype.GetSupertype(&superType);
				if(strcmp(superType.Type(),"audio") != 0)
				{
					beep();
					(new BAlert("","This is not a audio file.","OK",NULL,NULL,
							B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
					break;
				}
				// add file item
				BMessage *msg = new BMessage(M_ITEM_MESSAGE);
				BPath path(&ref);
				msg->AddRef("refs",&ref);
				if(!(menu->FindItem(path.Leaf())))
					menu->AddItem(new BMenuItem(path.Leaf(),msg),0);
				// refresh item
				HEventItem *item = cast_as(fEventList->ItemAt(sel),HEventItem);
				item->SetPath(path.Path());
				fEventList->InvalidateItem(sel);
				// check file menu		
				BMenuItem *menuitem = menu->FindItem(path.Leaf());
				if(menuitem)
					menuitem->SetMarked(true);
			}
			break;
		}
	case M_PLAY_MESSAGE:
		{
			int32 sel = fEventList->CurrentSelection();
			if(sel >= 0)
			{
				HEventItem *item = cast_as(fEventList->ItemAt(sel),HEventItem);
				//system_beep(item->Name());
				const char* path = item->Path();
				if(path)
				{
					entry_ref ref;
					::get_ref_for_path(path,&ref);
					/*BSound *sound = new BSound(&ref);
					if(sound->InitCheck() == B_OK)
					{
						fSoundPlayer.Start();
						fId = fSoundPlayer.StartPlaying(sound);
						sound->ReleaseRef();
					}*/
					
					//fSoundHandle = ::play_sound(&ref,false,false,true);
					delete fPlayer;
					fPlayer = new BFileGameSound(&ref,false);
					fPlayer->StartPlaying();
				}
			}
			break;
		}
	case M_STOP_MESSAGE:
		{
			//::stop_sound(fSoundHandle);
			if(!fPlayer)
				break;
			if(fPlayer->IsPlaying())
			{
				fPlayer->StopPlaying();
				delete fPlayer;
				fPlayer = NULL;
			}
			break;
		}
	case M_EVENT_CHANGED:
		{
			const char* path;
			BMenuField *menufield = cast_as(FindView("filemenu"),BMenuField);
			BMenu *menu = menufield->Menu();
			
			if(message->FindString("path",&path) == B_OK)
			{
				BPath path(path);
				if(path.InitCheck() != B_OK)
				{
					BMenuItem *item = menu->FindItem("<none>");
					if(item)
						item->SetMarked(true);
				}else{	
					BMenuItem *item = menu->FindItem(path.Leaf());
					if(item)
						item->SetMarked(true);
				}
			}
			break;
		}
	case M_ITEM_MESSAGE:
		{
			entry_ref ref;
			if(message->FindRef("refs",&ref) == B_OK)
			{
				int32 sel = fEventList->CurrentSelection();
				if(sel >= 0)
				{
					HEventItem *item = cast_as(fEventList->ItemAt(sel),HEventItem);
					item->SetPath(BPath(&ref).Path());
					fEventList->InvalidateItem(sel);
				}
			}
			break;
		}
	case M_NONE_MESSAGE:
		{
			int32 sel = fEventList->CurrentSelection();
			if(sel >= 0)
			{
				HEventItem *item = cast_as(fEventList->ItemAt(sel),HEventItem);
				item->SetPath(NULL);
				fEventList->InvalidateItem(sel);
			}	
			break;
		}
	default:
		_inherited::MessageReceived(message);
	}
}

/***********************************************************
 * Init menu
 ***********************************************************/
void
HWindow::SetupMenuField()
{
	BMenuField *menufield = cast_as(FindView("filemenu"),BMenuField);
	BMenu *menu = menufield->Menu();
	int32 count = fEventList->CountItems();
	for(register int32 i = 0;i < count;i++)
	{
		HEventItem *item = cast_as(fEventList->ItemAt(i),HEventItem);
		if(!item)
			continue;
			
		BPath path(item->Path());
		if(path.InitCheck() != B_OK)
			continue;
		if( menu->FindItem(path.Leaf()) )
			continue;
		
		BMessage *msg = new BMessage(M_ITEM_MESSAGE);
		entry_ref ref;
		::get_ref_for_path(path.Path(),&ref);
		msg->AddRef("refs",&ref);
		menu->AddItem(new BMenuItem(path.Leaf(),msg),0);			
	}
	
	BPath path("/boot/beos/etc/sounds");
	status_t err = B_OK;
	BDirectory dir( path.Path() );
	BEntry entry;
	BPath item_path;
   	while( err == B_OK ){
		err = dir.GetNextEntry( (BEntry*)&entry, TRUE );			
		if( entry.InitCheck() != B_NO_ERROR ){
			break;
		}
		entry.GetPath(&item_path);
		
		if( menu->FindItem(item_path.Leaf()) )
			continue;
			
		BMessage *msg = new BMessage(M_ITEM_MESSAGE);
		entry_ref ref;
		::get_ref_for_path(item_path.Path(),&ref);
		msg->AddRef("refs",&ref);
		menu->AddItem(new BMenuItem(item_path.Leaf(),msg),0);		
	}
	
	path.SetTo("/boot/home/config/sounds");
	dir.SetTo(path.Path());
	err = B_OK;
	while( err == B_OK ){
		err = dir.GetNextEntry( (BEntry*)&entry, TRUE );			
		if( entry.InitCheck() != B_NO_ERROR ){
			break;
		}
		entry.GetPath(&item_path);
		
		if( menu->FindItem(item_path.Leaf()) )
			continue;
			
		BMessage *msg = new BMessage(M_ITEM_MESSAGE);
		entry_ref ref;
		
		::get_ref_for_path(item_path.Path(),&ref);
		msg->AddRef("refs",&ref);
		menu->AddItem(new BMenuItem(item_path.Leaf(),msg),0);		
	}
	
	path.SetTo("/boot/home/media");
	dir.SetTo(path.Path());
	err = B_OK;
	while( err == B_OK ){
		err = dir.GetNextEntry( (BEntry*)&entry, TRUE );			
		if( entry.InitCheck() != B_NO_ERROR ){
			break;
		}
		entry.GetPath(&item_path);
		
		if( menu->FindItem(item_path.Leaf()) )
			continue;
			
		BMessage *msg = new BMessage(M_ITEM_MESSAGE);
		entry_ref ref;
		
		::get_ref_for_path(item_path.Path(),&ref);
		msg->AddRef("refs",&ref);
		menu->AddItem(new BMenuItem(item_path.Leaf(),msg),0);		
	}

}

/***********************************************************
 * Remove Event
 ***********************************************************/
void
HWindow::RemoveEvent()
{
	int32 sel = fEventList->CurrentSelection();
	if(sel < 0)
		return;
	// check system beep event
	HEventItem *item = cast_as(fEventList->ItemAt(sel),HEventItem);
	if(::strcmp(item->Name(),"Beep") == 0
		||::strcmp(item->Name(),"Startup")==0
		||::strcmp(item->Name(),"New E-mail") == 0)
	{
		beep();
		(new BAlert("","This is a system beep event.","OK"))->Go();
		return;
	}
	//
	int32 index = (new BAlert("","Would you like to remove this event?","OK","Cancel"))->Go();
	if(index == 0)
	{	
		if(item)
			fEventList->RemoveEvent(item);
	}
}

/***********************************************************
 * Pulse
 ***********************************************************/
void
HWindow::Pulse()
{
	int32 sel = fEventList->CurrentSelection();
	BMenuField *menufield = cast_as(FindView("filemenu"),BMenuField);
	BButton *button = cast_as(FindView("play"),BButton);
	BButton *stop = cast_as(FindView("stop"),BButton);
	
	if(!menufield)
		return;
	if(sel >=0)
	{
		menufield->SetEnabled(true);
		button->SetEnabled(true);
	}else{
		menufield->SetEnabled(false);
		button->SetEnabled(false);
	}
	if(fPlayer)
	{
		if(fPlayer->IsPlaying())
			stop->SetEnabled(true);
		else
			stop->SetEnabled(false);
	}else
		stop->SetEnabled(false);
}


/***********************************************************
 * DispatchMessage
 ***********************************************************/
void
HWindow::DispatchMessage(BMessage *message,BHandler *handler)
{
	if(message->what == B_PULSE)
		Pulse();
	BWindow::DispatchMessage(message,handler);
}

/***********************************************************
 * QuitRequested
 ***********************************************************/
bool
HWindow::QuitRequested()
{
	RectUtils utils;
	CLVColumn *col = fEventList->ColumnAt(1);
	float width = col->Width();
	
	app_info info; 
    be_app->GetAppInfo(&info); 
   	BEntry entry(&info.ref); 
   	
	BFile appfile(&entry,B_WRITE_ONLY);
	appfile.WriteAttr("event_width",B_FLOAT_TYPE,0,&width,sizeof(float));
	
	col = fEventList->ColumnAt(2);
	width = col->Width();
	appfile.WriteAttr("sound_width",B_FLOAT_TYPE,0,&width,sizeof(float));
	
	fEventList->RemoveAll();
	
	utils.SaveRectToApp("window_rect",this->Frame());
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
