// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        HWindow.cpp
//  Author:      Jérôme Duval, Oliver Ruiz Dorantes, Atsushi Takamatsu
//  Description: Sounds Preferences
//  Created :    November 24, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include "HWindow.h"
#include "HEventList.h"
#include "HEventItem.h"
#include <ScrollView.h>
#include <StringView.h>
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
	InitGUI();
	float min_width,min_height,max_width,max_height;
	GetSizeLimits(&min_width,&max_width,&min_height,&max_height);
	min_width = 300;
	min_height = 200;
	SetSizeLimits(min_width,max_width,min_height,max_height);
		
	fFilePanel = new BFilePanel();
	fFilePanel->SetTarget(this);
	
	fEventList->Select(0);
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
	rect.bottom -= 106;
	BView *view = new BView(rect,"",B_FOLLOW_ALL, B_WILL_DRAW|B_PULSE_NEEDED);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);
		
	BRect stringRect(16, 5, 60, 22);
	BStringView *stringView = new BStringView(stringRect, "event", "Event");
	stringView->SetFont(be_bold_font);
	view->AddChild(stringView);
	
	stringRect.OffsetBy(100, 0);
	stringView = new BStringView(stringRect, "sound", "Sound");
	stringView->SetFont(be_bold_font);
	view->AddChild(stringView);
		
	rect.left += 13;
	rect.right -= B_V_SCROLL_BAR_WIDTH + 13;
	rect.top += 28;
	rect.bottom -= 7;
	fEventList = new HEventList(rect);
	fEventList->SetType(BMediaFiles::B_SOUNDS);
	
	BScrollView *scrollView = new BScrollView("" 
									,fEventList
									,B_FOLLOW_ALL
									,0
									,false
									,true);
	view->AddChild(scrollView);
	
	rect = Bounds();
	rect.top = rect.bottom - 105;
	view = new BView(rect,"",B_FOLLOW_LEFT_RIGHT|B_FOLLOW_BOTTOM,B_WILL_DRAW|B_PULSE_NEEDED);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);
	rect = view->Bounds().InsetBySelf(12, 12);
	BBox *box = new BBox(rect,"",B_FOLLOW_ALL);
	view->AddChild(box);
	rect = box->Bounds();
	rect.top += 10;
	rect.left += 15;
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
	menuField->SetDivider(menuField->StringWidth("Sound File:")+10);
	box->AddChild(menuField);
	rect.OffsetBy(-2,38);
	rect.left = rect.right - 80;
	BButton *button = new BButton(rect
						,"stop"
						,"Stop"
						,new BMessage(M_STOP_MESSAGE)
						,B_FOLLOW_RIGHT|B_FOLLOW_TOP);
	box->AddChild(button);
	
	rect.OffsetBy(-90,0);
	button = new BButton(rect
								,"play"
								,"Play"
								,new BMessage(M_PLAY_MESSAGE)
								,B_FOLLOW_RIGHT|B_FOLLOW_TOP);
	box->AddChild(button);

	// setup file menu
	SetupMenuField();
	menu->FindItem("<none>")->SetMarked(true);
}


/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HWindow::MessageReceived(BMessage *message)
{
	switch(message->what) {
	case M_OTHER_MESSAGE:
		{
			BMenuField *menufield = cast_as(FindView("filemenu"),BMenuField);
			BMenu *menu = menufield->Menu();
			
			int32 sel = fEventList->CurrentSelection();
			if(sel >= 0) {
				HEventItem *item = cast_as(fEventList->ItemAt(sel),HEventItem);
				BPath path(item->Path());
				if(path.InitCheck() != B_OK) {
					BMenuItem *item = menu->FindItem("<none>");
					if(item)
						item->SetMarked(true);
				} else{	
					BMenuItem *item = menu->FindItem(path.Leaf());
					if(item)
						item->SetMarked(true);
				}
			}
			fFilePanel->Show();
			break;
		}
	case B_SIMPLE_DATA:
	case B_REFS_RECEIVED:
		{
			entry_ref ref;
			int32 sel = fEventList->CurrentSelection();
			if(message->FindRef("refs",&ref) == B_OK && sel >= 0) {
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
				if(strcmp(superType.Type(),"audio") != 0) {
					beep();
					(new BAlert("","This is not a audio file.","OK",NULL,NULL,
							B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
					break;
				}
				// add file item
				BMessage *msg = new BMessage(M_ITEM_MESSAGE);
				BPath path(&ref);
				msg->AddRef("refs",&ref);
				BMenuItem *menuitem = menu->FindItem(path.Leaf());
				if(!menuitem)
					menu->AddItem(menuitem = new BMenuItem(path.Leaf(),msg),0);
				// refresh item
				fEventList->SetPath(BPath(&ref).Path());
				// check file menu	
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
				const char* path = item->Path();
				if(path)
				{
					entry_ref ref;
					::get_ref_for_path(path,&ref);
					delete fPlayer;
					fPlayer = new BFileGameSound(&ref,false);
					fPlayer->StartPlaying();
				}
			}
			break;
		}
	case M_STOP_MESSAGE:
		{
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
			
			if(message->FindString("path",&path) == B_OK) {
				BPath path(path);
				if(path.InitCheck() != B_OK) {
					BMenuItem *item = menu->FindItem("<none>");
					if(item)
						item->SetMarked(true);
				} else {	
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
			if(message->FindRef("refs",&ref) == B_OK) {
				fEventList->SetPath(BPath(&ref).Path());
			}
			break;
		}
	case M_NONE_MESSAGE:
		{
			fEventList->SetPath(NULL);
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
	for(int32 i = 0; i < count; i++) {
		HEventItem *item = cast_as(fEventList->ItemAt(i), HEventItem);
		if(!item)
			continue;
			
		BPath path(item->Path());
		if(path.InitCheck() != B_OK)
			continue;
		if(menu->FindItem(path.Leaf()))
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
	if(sel >=0) {
		menufield->SetEnabled(true);
		button->SetEnabled(true);
	} else {
		menufield->SetEnabled(false);
		button->SetEnabled(false);
	}
	if(fPlayer) {
		if(fPlayer->IsPlaying())
			stop->SetEnabled(true);
		else
			stop->SetEnabled(false);
	} else
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
	fEventList->RemoveAll();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
