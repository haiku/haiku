//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		CurView.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	cursor handler for the app
//  
//------------------------------------------------------------------------------
#include <OS.h>
#include <Directory.h>
#include <Alert.h>
#include <storage/Path.h>
#include <Entry.h>
#include <File.h>
#include <stdio.h>
#include "CursorWhichItem.h"
#include "CurView.h"
#include <PortLink.h>
#include "defs.h"
#include "ServerConfig.h"
#include <InterfaceDefs.h>

//#define DEBUG_CURSORSET

#define SAVE_CURSORSET 'svcu'
#define DELETE_CURSORSET 'dlcu'
#define LOAD_CURSORSET 'ldcu'

CurView::CurView(const BRect &frame, const char *name, int32 resize, int32 flags)
	:BView(frame,name,resize,flags), settings(B_SIMPLE_DATA)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BMenuBar *mb=new BMenuBar(BRect(0,0,Bounds().Width(),16),"menubar");

	settings_menu=new BMenu("Settings");
	settings_menu->AddItem(new BMenuItem("Save Cursor Set",new BMessage(SAVE_CURSORSET),'S'));
	settings_menu->AddSeparatorItem();
	settings_menu->AddItem(new BMenuItem("Delete Cursor Set",new BMessage(DELETE_CURSORSET)));
	mb->AddItem(settings_menu);

	cursorset_menu=LoadCursorSets();
	if(cursorset_menu)
		mb->AddItem(cursorset_menu);
	else
	{
		// We should *never* be here, but just in case....
		cursorset_menu=new BMenu("Cursor Sets");
		mb->AddItem(cursorset_menu);
	}
	AddChild(mb);

	BRect wellrect(0,0,20,20);
	wellrect.OffsetTo(10,25);
	wellrect.right=wellrect.left+50;
	cursorset_label=new BStringView(wellrect,"cursorset_label","Cursor Set: ");
	AddChild(cursorset_label);
	cursorset_name="<untitled>";


	// Set up list of cursor attributes
	BRect rect(10,60,200,160);
	attrlist=new BListView(rect,"AttributeList");
	
	scrollview=new BScrollView("ScrollView",attrlist, B_FOLLOW_LEFT |
		B_FOLLOW_TOP, 0, false, true);
	AddChild(scrollview);
	scrollview->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	attrlist->SetSelectionMessage(new BMessage(ATTRIBUTE_CHOSEN));

	attrlist->AddItem(new CursorWhichItem(B_CURSOR_DEFAULT));
	attrlist->AddItem(new CursorWhichItem(B_CURSOR_TEXT));
	attrlist->AddItem(new CursorWhichItem(B_CURSOR_MOVE));
	attrlist->AddItem(new CursorWhichItem(B_CURSOR_DRAG));
	attrlist->AddItem(new CursorWhichItem(B_CURSOR_RESIZE));
	attrlist->AddItem(new CursorWhichItem(B_CURSOR_RESIZE_NWSE));
	attrlist->AddItem(new CursorWhichItem(B_CURSOR_RESIZE_NESW));
	attrlist->AddItem(new CursorWhichItem(B_CURSOR_RESIZE_NS));
	attrlist->AddItem(new CursorWhichItem(B_CURSOR_RESIZE_EW));

	BRect cvrect;
	
	cvrect.Set(0,0,50,25);
	cvrect.OffsetBy(scrollview->Frame().right+20,
		scrollview->Frame().bottom-cvrect.Height());

	defaults=new BButton(cvrect,"DefaultsButton","Defaults",
		new BMessage(DEFAULT_SETTINGS),B_FOLLOW_LEFT |B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE);
	AddChild(defaults);

	cvrect.OffsetBy(70,0);
	revert=new BButton(cvrect,"RevertButton","Revert",
		new BMessage(REVERT_SETTINGS),B_FOLLOW_LEFT |B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE);
	AddChild(revert);
	revert->SetEnabled(false);
	
	cvrect.OffsetBy(70,0);
	apply=new BButton(cvrect,"ApplyButton","Apply",
		new BMessage(APPLY_SETTINGS),B_FOLLOW_LEFT |B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE);
	AddChild(apply);
	apply->SetEnabled(false);

	BEntry entry(COLOR_SET_DIR);
	entry_ref ref;
	entry.GetRef(&ref);
	savepanel=new BFilePanel(B_SAVE_PANEL, NULL,
		&ref, 0, false);

	attribute=B_PANEL_BACKGROUND_COLOR;
	attrstring="Background";
	LoadSettings();

}

CurView::~CurView(void)
{
	delete savepanel;
}

void CurView::AllAttached(void)
{
	attrlist->Select(0);
	attrlist->SetTarget(this);
	apply->SetTarget(this);
	defaults->SetTarget(this);
	revert->SetTarget(this);
	settings_menu->SetTargetForItems(this);
	cursorset_menu->SetTargetForItems(this);

	BMessenger msgr(this);
	savepanel->SetTarget(msgr);
}

void CurView::MessageReceived(BMessage *msg)
{
	if(msg->WasDropped())
	{
	}

	switch(msg->what)
	{
/*		case DELETE_CURSORSET:
		{
			// Construct the path and delete
			BString path(COLOR_SET_DIR);
			path+=cursorset_name;
			
			BString printstring("Remove ");
			printstring+=cursorset_name;
			printstring+=" from disk permenantly?";
			BAlert *a=new BAlert("OpenBeOS",printstring.String(),"Yes", "No");
			if(a->Go()==0)
			{
				int stat=remove(path.String());
				if(stat!=0)
				{
#ifdef DEBUG_CURSORSET
printf("MSG: Delete Request - couldn't delete file %s\n",path.String());
#endif
				}
				else
				{
					BMenuItem *item=cursorset_menu->FindItem(cursorset_name.String());
					if(item!=NULL)
					{
						if(cursorset_menu->RemoveItem(item))
							delete item;
					}
				}
			}
			break;
		}
		case LOAD_CURSORSET:
		{
			BString name;
			if(msg->FindString("name",&name)!=B_OK)
			{
#ifdef DEBUG_CURSORSET
printf("MSG: Load Request - couldn't find file name\n");
#endif
				break;
			}
			LoadCursorSet(name);
			break;
		}
		case SAVE_CURSORSET:
		{
			savepanel->Show();
			break;
		}
		case B_SAVE_REQUESTED:
		{
			BString name;
			if(msg->FindString("name",&name)!=B_OK)
			{
#ifdef DEBUG_CURSORSET
printf("MSG: Save Request - couldn't find file name\n");
#endif
				break;
			}
			SaveCursorSet(name);
			break;
		}
		case ATTRIBUTE_CHOSEN:
		{
			break;
		}
		case APPLY_SETTINGS:
		{
			SaveSettings();
			NotifyServer();
			break;
		}
		case TRY_SETTINGS:
		{
			// Tell server to apply settings here without saving them.
			// Theoretically, the user can set this temporarily and keep it
			// until the next boot.
			NotifyServer();
			break;
		}
		case REVERT_SETTINGS:
		{
			if(prev_set_name=="Default")
				SetDefaults();
			
			LoadCursorSet(prev_set_name);
			break;
		}
		case DEFAULT_SETTINGS:
		{
			break;
		}
*/		default:
			BView::MessageReceived(msg);
			break;
	}
}

BMenu *CurView::LoadCursorSets(void)
{
#ifdef DEBUG_CURSORSET
printf("Loading cursor sets from disk\n");
#endif
	// This function populates the member menu *cursorset_menu with the cursor
	// set files located in the cursor set directory. To ensure that there are
	// no entries pointing to invalid cursor sets, they are validated before
	// a menu item is added.
	BDirectory dir;
	BEntry entry;
	BPath path;
	BString name;

	BMenu *menu=new BMenu("Cursor Sets");

	status_t dirstat=dir.SetTo(COLOR_SET_DIR);
	if(dirstat!=B_OK)
	{
		// We couldn't set the directory, so more than likely it just
		// doesn't exist. Create it and return an empty menu.
		switch(dirstat)
		{
			case B_NAME_TOO_LONG:
			{
				BAlert *a=new BAlert("OpenBeOS","Couldn't open the folder for cursor sets. "
					"You will be able to change system cursors, but be unable to save them to a cursor set. "
					"Please contact OpenBeOS about Appearance Preferences::CurView::"
					"LoadCursorSets::B_NAME_TOO_LONG for a bugfix", "OK",
					 NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				a->Go();
				break;
			}
			case B_ENTRY_NOT_FOUND:
			{
				create_directory(COLOR_SET_DIR,0777);
				break;
			}
			case B_BAD_VALUE:
			{
				printf("CurView::LoadCursorSets(): Invalid cursorset folder path.\n");
				break;
			}
			case B_NO_MEMORY:
			{
				printf("CurView::LoadCursorSets(): No memory left. We're probably going to crash now. \n");
				break;
			}
			case B_BUSY:
			{
				printf("CurView::LoadCursorSets(): Busy node " CURSOR_SET_DIR "\n");
				break;
			}
			case B_FILE_ERROR:
			{
				BAlert *a=new BAlert("OpenBeOS","Couldn't open the folder for cursor sets "
					"because of a file error. Perhaps there is a file (instead of a folder) at " COLOR_SET_DIR
					"? You will be able to change system cursors, but be unable to save them to a cursor set. ",
					"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				a->Go();
				break;
			}
			case B_NO_MORE_FDS:
			{
				BAlert *a=new BAlert("OpenBeOS","Couldn't open the folder for cursor sets "
					"because there are too many open files. Please close some files and restart "
					" this application.", "OK",
					 NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				a->Go();
				if(Window())
					Window()->PostMessage(B_QUIT_REQUESTED);
				break;
			}
		}
		
		return menu;
	}

	int32 count=dir.CountEntries();

	BMessage *msg;
	for(int32 i=0;i<count;i++)
	{
		dir.GetNextEntry(&entry);
		entry.GetPath(&path);
		
		name=path.Path();
		name.Remove(0,name.FindLast('/')+1);

		msg=new BMessage(LOAD_CURSORSET);
		msg->AddString("name",name);
		menu->AddItem(new BMenuItem(name.String(),msg));
	}
	
	return menu;
}	

void CurView::LoadCursorSet(const BString &name)
{
/*
	// Load the current GUI cursor settings from a cursor set file.

#ifdef DEBUG_CURSORSET
printf("LoadCursorSet: %s\n",name.String());
#endif

	BDirectory dir,newdir;
	if(dir.SetTo(CURSOR_SET_DIR)==B_ENTRY_NOT_FOUND)
	{
#ifdef DEBUG_CURSORSET
printf("Cursor set folder not found. Creating %s\n",COLOR_SET_DIR);
#endif
		create_directory(COLOR_SET_DIR,0777);
	}

	BString path(COLOR_SET_DIR);
	path+=name.String();
	BFile file(path.String(),B_READ_ONLY);

	if(file.InitCheck()!=B_OK)
	{
#ifdef DEBUG_CURSORSET
printf("Couldn't open file %s for read\n",path.String());
#endif
		return;
	}
	if(settings.Unflatten(&file)==B_OK)
	{
#ifdef DEBUG_CURSORSET
settings.PrintToStream();
#endif
		BString internal_name;
		settings.FindString("name",&internal_name);
//		BString namestr("Cursor Set: ");
//		namestr+=internal_name.String();
//		cursorset_label->SetText(namestr.String());
		SetCursorSetName(internal_name.String());
		return;
	}
#ifdef DEBUG_CURSORSET
printf("Error unflattening file %s\n",name.String());
#endif
*/
}

void CurView::SaveCursorSet(const BString &name)
{
/*
	// Save the current cursor attributes as a flattened BMessage in the 
	// cursor set folder
	BString path(COLOR_SET_DIR);
	path+=name.String();

#ifdef DEBUG_CURSORSET
printf("SaveCursorSet: %s\n",path.String());
#endif

	if(settings.ReplaceString("name",name.String())!=B_OK)
	{
#ifdef DEBUG_CURSORSET
printf("SaveCursorSet: Couldn't replace set name in settings\n");
#endif
	}

	BFile file(path.String(),B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);

	if(file.InitCheck()!=B_OK)
	{
#ifdef DEBUG_CURSORSET
printf("SaveCursorSet: Couldn't open settings file for write\n");
#endif
	}
	
	if(settings.Flatten(&file)!=B_OK)
	{
#ifdef DEBUG_CURSORSET
printf("SaveCursorSet: Couldn't flatten settings to file\n");
#endif
		return;
	}

	BMessage *msg=new BMessage(LOAD_CURSORSET);
	msg->AddString("name",name.String());

	if(cursorset_menu->AddItem(new BMenuItem(name.String(),msg))==false)
	{
#ifdef DEBUG_CURSORSET
printf("SaveCursorSet: Error in adding item to menu\n");
#endif
	}
	SetCursorSetName(name.String());
*/
}

void CurView::SetCursorSetName(const char *name)
{
	if(!name)
		return;
	BString namestr("Cursor Set: ");
	cursorset_name=name;
	namestr+=name;
	cursorset_label->SetText(namestr.String());
	cursorset_label->Invalidate();
}

void CurView::SaveSettings(void)
{
	// Save the current GUI cursor settings to the GUI cursors file in the 
	// path specified in defs.h
	
	BString path(SERVER_SETTINGS_DIR);
	path+=CURSOR_SETTINGS_NAME;
#ifdef DEBUG_CURSORSET
printf("SaveSettings: %s\n",path.String());
#endif
	BFile file(path.String(),B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);
	
	settings.Flatten(&file);
	prev_set_name=cursorset_name;
	revert->SetEnabled(false);
	revert->SetEnabled(false);
}

void CurView::LoadSettings(void)
{
/*	// Load the current GUI cursor settings from disk. This is done instead of
	// getting them from the server at this point for testing purposes. Comment
	// out the #define LOAD_SETTINGS_FROM_DISK line to use the server query code
#ifdef DEBUG_CURSORSET
printf("Loading settings from disk\n");
#endif
	settings.MakeEmpty();

	BDirectory dir,newdir;
	if(dir.SetTo(SERVER_SETTINGS_DIR)==B_ENTRY_NOT_FOUND)
	{
#ifdef DEBUG_CURSORSET
printf("Cursor set folder not found. Creating %s\n",SERVER_SETTINGS_DIR);
#endif
		create_directory(SERVER_SETTINGS_DIR,0777);
	}

	BString path(SERVER_SETTINGS_DIR);
	path+=COLOR_SETTINGS_NAME;
	BFile file(path.String(),B_READ_ONLY);

	if(file.InitCheck()!=B_OK)
	{
#ifdef DEBUG_CURSORSET
printf("Couldn't open file %s for read\n",path.String());
#endif
		SetDefaults();
		SaveSettings();
		return;
	}
	if(settings.Unflatten(&file)==B_OK)
	{
		settings.FindString("name",&cursorset_name);
		SetCursorSetName(cursorset_name.String());
		prev_set_name=cursorset_name;
		picker->SetValue(GetCursorFromMessage(&settings,attrstring.String()));
#ifdef DEBUG_CURSORSET
settings.PrintToStream();
#endif
		return;
	}
#ifdef DEBUG_CURSORSET
printf("Error unflattening SystemCursors file %s\n",path.String());
#endif
	
	// If we get this far, we have encountered an error, so reset the settings
	// to the defaults
	SetDefaults();
	SaveSettings();
*/
}

void CurView::SetDefaults(void)
{
/*
#ifdef DEBUG_CURSORSET
printf("Initializing cursor settings to defaults\n");
#endif
	settings.MakeEmpty();
	settings.AddString("name","Default");
	cursorset_name="Default";

	ColorWhichItem whichitem(B_PANEL_BACKGROUND_COLOR);
	rgb_color col={216,216,216,255};
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,0);
	whichitem.SetAttribute((color_which)B_PANEL_TEXT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,255,255,255);
	whichitem.SetAttribute((color_which)B_DOCUMENT_BACKGROUND_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,0);
	whichitem.SetAttribute((color_which)B_DOCUMENT_TEXT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,245,245,245);
	whichitem.SetAttribute((color_which)B_CONTROL_BACKGROUND_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,0);
	whichitem.SetAttribute((color_which)B_CONTROL_TEXT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,0);
	whichitem.SetAttribute((color_which)B_CONTROL_BORDER_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,115,120,184);
	whichitem.SetAttribute((color_which)B_CONTROL_HIGHLIGHT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,170,50,184);
	whichitem.SetAttribute((color_which)B_NAVIGATION_BASE_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,0);
	whichitem.SetAttribute((color_which)B_NAVIGATION_PULSE_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,255,255,255);
	whichitem.SetAttribute((color_which)B_SHINE_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,0);
	whichitem.SetAttribute((color_which)B_SHADOW_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,0);
	whichitem.SetAttribute((color_which)B_MENU_SELECTED_BORDER_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,255,255,0);
	whichitem.SetAttribute((color_which)B_TOOLTIP_BACKGROUND_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,0);
	whichitem.SetAttribute((color_which)B_TOOLTIP_TEXT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,255,0);
	whichitem.SetAttribute((color_which)B_SUCCESS_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,255,0,0);
	whichitem.SetAttribute((color_which)B_FAILURE_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,51,102,160);
	whichitem.SetAttribute((color_which)B_MENU_SELECTED_BACKGROUND_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	whichitem.SetAttribute(B_PANEL_BACKGROUND_COLOR);
	SetRGBColor(&col,216,216,216);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,216,216,216,0);
	whichitem.SetAttribute(B_MENU_BACKGROUND_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,0);
	whichitem.SetAttribute(B_MENU_ITEM_TEXT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,255,255,255);
	whichitem.SetAttribute(B_MENU_SELECTED_ITEM_TEXT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,255,203,0);
	whichitem.SetAttribute(B_WINDOW_TAB_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,0);
	whichitem.SetAttribute((color_which)B_WINDOW_TAB_TEXT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,232,232,232);
	whichitem.SetAttribute((color_which)B_INACTIVE_WINDOW_TAB_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,80,80,80);
	whichitem.SetAttribute((color_which)B_INACTIVE_WINDOW_TAB_TEXT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

//	BString labelstr("Color Set: ");
//	labelstr+=cursorset_name;
//	cursorset_label->SetText(labelstr.String());
	SetCursorSetName("Default");
*/
}

void CurView::NotifyServer(void)
{
}

rgb_color CurView::GetColorFromMessage(BMessage *msg, const char *name, int32 index=0)
{
/*	
	// Simple function to do the dirty work of getting an rgb_color from
	// a message
	rgb_color *col,rcolor={0,0,0,0};
	ssize_t size;

	if(!msg || !name)
		return rcolor;

	if(msg->FindData(name,(type_code)'RGBC',index,(const void**)&col,&size)==B_OK)
		rcolor=*col;
	return rcolor;
*/
}
