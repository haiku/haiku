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
//	File Name:		APRView.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	color handler for the app
//  
//	Handles all the grunt work. Color settings are stored in a BMessage to
//	allow for easy transition to and from disk. The messy details are in the
//	hard-coded strings to translate the enumerated color_which defines to
//	strings and such, but even those should be easy to maintain.
//------------------------------------------------------------------------------
#include <OS.h>
#include <Directory.h>
#include <Alert.h>
#include <storage/Path.h>
#include <Entry.h>
#include <File.h>
#include <Screen.h>	// for settings Desktop color
#include <stdio.h>

#include "APRView.h"
#include <PortLink.h>
#include <PortMessage.h>
#include "defs.h"
#include "ColorWell.h"
#include <ColorUtils.h>
#include <InterfaceDefs.h>
#include <ServerProtocol.h>
#include "ColorWhichItem.h"
#include "ServerConfig.h"

//#define DEBUG_COLORSET

#ifdef DEBUG_COLORSET
#define STRACE(a) printf a
#else
#define STRACE(A) /* nothing */
#endif

//#define SAVE_COLORSET 'svcs'
//#define DELETE_COLORSET 'dlcs'
//#define LOAD_COLORSET 'ldcs'
#define COLOR_DROPPED 'cldp'

APRView::APRView(const BRect &frame, const char *name, int32 resize, int32 flags)
	:BView(frame,name,resize,flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	currentset=new ColorSet;
	prevset=NULL;
	
/*	BMenuBar *mb=new BMenuBar(BRect(0,0,Bounds().Width(),16),"menubar");

	settings_menu=new BMenu("Settings");
	settings_menu->AddItem(new BMenuItem("Save Color Set",new BMessage(SAVE_COLORSET),'S'));
	settings_menu->AddSeparatorItem();
	settings_menu->AddItem(new BMenuItem("Delete Color Set",new BMessage(DELETE_COLORSET)));
	mb->AddItem(settings_menu);

	colorset_menu=LoadColorSets();
	if(colorset_menu)
		mb->AddItem(colorset_menu);
	else
	{
		// We should *never* be here, but just in case....
		colorset_menu=new BMenu("Color Sets");
		mb->AddItem(colorset_menu);
	}
	AddChild(mb);
*/
	BRect wellrect(0,0,20,20);
	wellrect.OffsetTo(Bounds().Width()-(wellrect.Width()+10),25);

	colorwell=new ColorWell(wellrect,new BMessage(COLOR_DROPPED));
	AddChild(colorwell);

	wellrect.OffsetTo(10,25);
	wellrect.right=colorwell->Frame().left - 20;
//	colorset_label=new BStringView(wellrect,"colorset_label","Color Set: ");
//	AddChild(colorset_label);
//	colorset_label->ResizeToPreferred();

	// Set up list of color attributes
	BRect rect(10,10,200,100);
	attrlist=new BListView(rect,"AttributeList");
	
	scrollview=new BScrollView("ScrollView",attrlist, B_FOLLOW_LEFT |
		B_FOLLOW_TOP, 0, false, true);
	AddChild(scrollview);
	scrollview->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	attrlist->SetSelectionMessage(new BMessage(ATTRIBUTE_CHOSEN));

	attrlist->AddItem(new ColorWhichItem(B_PANEL_BACKGROUND_COLOR));
	attrlist->AddItem(new ColorWhichItem((color_which)B_PANEL_TEXT_COLOR));
	attrlist->AddItem(new ColorWhichItem((color_which)B_DOCUMENT_BACKGROUND_COLOR));
	attrlist->AddItem(new ColorWhichItem((color_which)B_DOCUMENT_TEXT_COLOR));
	attrlist->AddItem(new ColorWhichItem((color_which)B_CONTROL_BACKGROUND_COLOR));
	attrlist->AddItem(new ColorWhichItem((color_which)B_CONTROL_TEXT_COLOR));
	attrlist->AddItem(new ColorWhichItem((color_which)B_CONTROL_BORDER_COLOR));
	attrlist->AddItem(new ColorWhichItem((color_which)B_CONTROL_HIGHLIGHT_COLOR));
	attrlist->AddItem(new ColorWhichItem((color_which)B_NAVIGATION_BASE_COLOR));
	attrlist->AddItem(new ColorWhichItem((color_which)B_NAVIGATION_PULSE_COLOR));
	attrlist->AddItem(new ColorWhichItem((color_which)B_SHINE_COLOR));
	attrlist->AddItem(new ColorWhichItem((color_which)B_SHADOW_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_MENU_BACKGROUND_COLOR));
	attrlist->AddItem(new ColorWhichItem((color_which)B_MENU_SELECTED_BACKGROUND_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_MENU_ITEM_TEXT_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_MENU_SELECTED_ITEM_TEXT_COLOR));
	attrlist->AddItem(new ColorWhichItem((color_which)B_MENU_SELECTED_BORDER_COLOR));
	attrlist->AddItem(new ColorWhichItem((color_which)B_TOOLTIP_BACKGROUND_COLOR));
	
	attrlist->AddItem(new ColorWhichItem((color_which)B_SUCCESS_COLOR));
	attrlist->AddItem(new ColorWhichItem((color_which)B_FAILURE_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_WINDOW_TAB_COLOR));
	attrlist->AddItem(new ColorWhichItem((color_which)B_WINDOW_TAB_TEXT_COLOR));
	attrlist->AddItem(new ColorWhichItem((color_which)B_INACTIVE_WINDOW_TAB_COLOR));
	attrlist->AddItem(new ColorWhichItem((color_which)B_INACTIVE_WINDOW_TAB_TEXT_COLOR));

	picker=new BColorControl(BPoint(10,scrollview->Frame().bottom+20),B_CELLS_32x8,5.0,"Picker",
		new BMessage(UPDATE_COLOR));
	AddChild(picker);
	
	BRect cvrect;
	
	cvrect=picker->Frame();
	cvrect.right=cvrect.left+60;
	cvrect.bottom=cvrect.top+30;
	
	// 40 is 10 pixel space between buttons and other objects, such as the edge or another button
	cvrect.OffsetTo( (Bounds().Width()-((cvrect.Width()*3)+40))/2,picker->Frame().bottom+10);

	defaults=new BButton(cvrect,"DefaultsButton","Defaults",
		new BMessage(DEFAULT_SETTINGS),B_FOLLOW_LEFT |B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE);
	AddChild(defaults);
	defaults->SetEnabled(false);

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
//	savepanel=new BFilePanel(B_SAVE_PANEL, NULL, &ref, 0, false);

	attribute=B_PANEL_BACKGROUND_COLOR;
	attrstring="Panel Background";
	LoadSettings();
}

APRView::~APRView(void)
{
//	delete savepanel;
	if(currentset)
		delete currentset;
	if(prevset)
		delete prevset;
}

void APRView::AllAttached(void)
{
	picker->SetTarget(this);
	attrlist->Select(0);
	attrlist->SetTarget(this);
	apply->SetTarget(this);
	defaults->SetTarget(this);
	revert->SetTarget(this);
//	settings_menu->SetTargetForItems(this);
//	colorset_menu->SetTargetForItems(this);
	colorwell->SetTarget(this);

	BMessenger msgr(this);
//	savepanel->SetTarget(msgr);
	picker->SetValue(currentset->StringToColor(attrstring.String()).GetColor32());
}

void APRView::MessageReceived(BMessage *msg)
{
	if(msg->WasDropped())
	{
		rgb_color *col;
		ssize_t size;
		if(msg->FindData("RGBColor",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		{
			picker->SetValue(*col);
			colorwell->SetColor(*col);
			colorwell->Invalidate();
		}
	}

	switch(msg->what)
	{
/*		case DELETE_COLORSET:
		{
			// We can't delete the Default set
			if(currentset->name.Compare("Default")==0)
				break;
			
			// Construct the path and delete
			BString path(COLOR_SET_DIR);
			path+=currentset->name;
			
			BString printstring("Remove ");
			printstring+=currentset->name;
			printstring+=" from disk permenantly?";
			BAlert *a=new BAlert("OpenBeOS",printstring.String(),"Yes", "No");
			if(a->Go()==0)
			{
				int stat=remove(path.String());
				if(stat!=0)
				{
					STRACE(("MSG: Delete Request - couldn't delete file %s\n",path.String()));
				}
				else
				{
					BMenuItem *item=colorset_menu->FindItem(currentset->name.String());
					if(item!=NULL)
					{
						if(colorset_menu->RemoveItem(item))
							delete item;
					}
				}
			}
			break;
		}
		case LOAD_COLORSET:
		{
			BString name;
			if(msg->FindString("name",&name)!=B_OK)
			{
				STRACE(("MSG: Load Request - couldn't find file name\n"));
				break;
			}
			LoadColorSet(name);
			break;
		}
		case SAVE_COLORSET:
		{
			// Saving the Default set is rather dumb
			if(currentset->name.Compare("Default")==0)
				break;
			
			savepanel->Show();
			break;
		}
		case B_SAVE_REQUESTED:
		{
			BString name;
			if(msg->FindString("name",&name)!=B_OK)
			{
				STRACE(("MSG: Save Request - couldn't find file name\n"));
				break;
			}
			if(name.ICompare("Default")==0)
			{
				BAlert *a=new BAlert("OpenBeOS","The name 'Default' is reserved. Please choose another.","OK");
				a->Go();
				savepanel->Show();
				break;
			}
			SaveColorSet(name);
			break;
		}
*/		case UPDATE_COLOR:
		{
			// Received from the color picker when its color changes
			if(!prevset)
			{
				prevset=new ColorSet;
				*prevset=*currentset;
			}
			
			rgb_color col=picker->ValueAsColor();

			colorwell->SetColor(col);
			colorwell->Invalidate();
			
			// Update current attribute in the settings
			if(currentset->SetColor(attrstring.String(),col)!=B_OK)
			{
				STRACE(("Couldn't set color for attribute %s\n",attrstring.String()));
			}
			
			if(!apply->IsEnabled())
				apply->SetEnabled(true);
			if(!defaults->IsEnabled())
				defaults->SetEnabled(true);
			if(!revert->IsEnabled())
				revert->SetEnabled(true);
			
//			SetColorSetName("<untitled>");
			Window()->PostMessage(SET_UI_COLORS);
			break;
		}
		case ATTRIBUTE_CHOSEN:
		{
			// Received when the user chooses a GUI attribute from the list
			ColorWhichItem *whichitem=(ColorWhichItem*)attrlist->ItemAt(attrlist->CurrentSelection());
			if(!whichitem)
				break;
			attrstring=whichitem->Text();
			UpdateControlsFromAttr(whichitem->Text());
			break;
		}
		case APPLY_SETTINGS:
		{
			if(prevset)
			{
				delete prevset;
				prevset=NULL;
				revert->SetEnabled(false);
			}
			if(currentset->name=="Default")
				defaults->SetEnabled(false);
			apply->SetEnabled(false);
			SaveSettings();
			NotifyServer();
			break;
		}
/*		case TRY_SETTINGS:
		{
			// Tell server to apply settings here without saving them.
			// Theoretically, the user can set this temporarily and keep it
			// until the next boot.
			NotifyServer();
			break;
		}
*/		case REVERT_SETTINGS:
		{
			delete currentset;
			currentset=prevset;
			prevset=NULL;
			
			rgb_color col=picker->ValueAsColor();

			colorwell->SetColor(col);
			colorwell->Invalidate();
			
			// Update current attribute in the settings
			currentset->SetColor(attrstring.String(),col);
			
			if(!apply->IsEnabled())
				apply->SetEnabled(false);
			if((currentset->name!="Default"))
				defaults->SetEnabled(true);
			else
				defaults->SetEnabled(false);
			revert->SetEnabled(false);
			
//			SetColorSetName(currentset->name.String());
			Window()->PostMessage(SET_UI_COLORS);
			break;
		}
		case DEFAULT_SETTINGS:
		{
			if(!prevset)
			{
				prevset=new ColorSet;
				*prevset=*currentset;
				revert->SetEnabled(true);
				apply->SetEnabled(true);
			}
			SetDefaults();
			UpdateControlsFromAttr(attrstring.String());
			if(Window())
				Window()->PostMessage(SET_UI_COLORS);
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}

BMenu *APRView::LoadColorSets(void)
{

	STRACE(("Loading color sets from disk\n"));

	// This function populates the member menu *colorset_menu with the color
	// set files located in the color set directory. To ensure that there are
	// no entries pointing to invalid color sets, they are validated before
	// a menu item is added.
	BDirectory dir;
	BEntry entry;
	BPath path;
	BString name;

	BMenu *menu=new BMenu("Color Sets");

	status_t dirstat=dir.SetTo(COLOR_SET_DIR);
	if(dirstat!=B_OK)
	{
		// We couldn't set the directory, so more than likely it just
		// doesn't exist. Create it and return an empty menu.
		switch(dirstat)
		{
			case B_NAME_TOO_LONG:
			{
				BAlert *a=new BAlert("OpenBeOS","Couldn't open the folder for color sets. "
					"You will be able to change system colors, but be unable to save them to a color set. "
					"Please contact OpenBeOS about Appearance Preferences::APRView::"
					"LoadColorSets::B_NAME_TOO_LONG for a bugfix", "OK",
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
				STRACE(("APRView::LoadColorSets(): Invalid colorset folder path.\n"));
				break;
			}
			case B_NO_MEMORY:
			{
				STRACE(("APRView::LoadColorSets(): No memory left. We're probably going to crash now. \n"));
				break;
			}
			case B_BUSY:
			{
				STRACE(("APRView::LoadColorSets(): Busy node " COLOR_SET_DIR "\n"));
				break;
			}
			case B_FILE_ERROR:
			{
				BAlert *a=new BAlert("OpenBeOS","Couldn't open the folder for color sets "
					"because of a file error. Perhaps there is a file (instead of a folder) at " COLOR_SET_DIR
					"? You will be able to change system colors, but be unable to save them to a color set. ",
					"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				a->Go();
				break;
			}
			case B_NO_MORE_FDS:
			{
				BAlert *a=new BAlert("OpenBeOS","Couldn't open the folder for color sets "
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

/*	int32 count=dir.CountEntries();

	BMessage *msg;
	for(int32 i=0;i<count;i++)
	{
		dir.GetNextEntry(&entry);
		entry.GetPath(&path);
		
		name=path.Path();
		name.Remove(0,name.FindLast('/')+1);

		msg=new BMessage(LOAD_COLORSET);
		msg->AddString("name",name);
		menu->AddItem(new BMenuItem(name.String(),msg));
	}
*/	
	return menu;
}	

void APRView::LoadColorSet(const BString &name)
{
	// Load the current GUI color settings from a color set file.
	
	STRACE(("LoadColorSet: %s\n",name.String()));
	
	BDirectory dir,newdir;
	if(dir.SetTo(COLOR_SET_DIR)==B_ENTRY_NOT_FOUND)
	{
		STRACE(("Color set folder not found. Creating %s\n",COLOR_SET_DIR));
		create_directory(COLOR_SET_DIR,0777);
	}
	
	BString path(COLOR_SET_DIR);
	path+=name.String();
	BFile file(path.String(),B_READ_ONLY);
	
	if(file.InitCheck()!=B_OK)
	{
		STRACE(("Couldn't open file %s for read\n",path.String()));
		return;
	}
	BMessage settings;
	
	if(settings.Unflatten(&file)!=B_OK)
	{
		STRACE(("Error unflattening file %s\n",name.String()));
		return;
	}
	currentset->ConvertFromMessage(&settings);
//	SetColorSetName(currentset->name.String());
	
	UpdateControlsFromAttr(attrstring.String());
}

/*
void APRView::SaveColorSet(const BString &name)
{

	// Save the current color attributes as a flattened BMessage in the 
	// color set folder
	BString path(COLOR_SET_DIR);
	path+=name.String();


	STRACE(("SaveColorSet: %s\n",path.String()));


	BFile file(path.String(),B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);

	if(file.InitCheck()!=B_OK)
	{
		STRACE(("SaveColorSet: Couldn't open settings file for write\n"));
		return;
	}
	
	BMessage settings;
	currentset->name=name;
	currentset->ConvertToMessage(&settings);

	if(settings.Flatten(&file)!=B_OK)
	{
		STRACE(("SaveColorSet: Couldn't flatten settings to file\n"));
		return;
	}

	BMessage *msg=new BMessage(LOAD_COLORSET);
	msg->AddString("name",name.String());

	if(colorset_menu->AddItem(new BMenuItem(name.String(),msg))==false)
	{
		STRACE(("SaveColorSet: Error in adding item to menu\n"));
	}
	SetColorSetName(name.String());
	
	// If we saved the color set after Applying it, the name won't be updated on disk, so 
	// save again to disk if this has happened.
	if(prevset==NULL)
		SaveSettings();
}

void APRView::SetColorSetName(const char *name)
{
	if(!name)
		return;
	BString namestr("Color Set: ");
	currentset->name.SetTo(name);
	namestr+=name;
	colorset_label->SetText(namestr.String());
	colorset_label->ResizeToPreferred();
	colorset_label->Invalidate();
}
*/
void APRView::SaveSettings(void)
{
	// Save the current GUI color settings to the GUI colors file in the 
	// path specified in defs.h
	
	BString path(SERVER_SETTINGS_DIR);
	path+=COLOR_SETTINGS_NAME;

	STRACE(("SaveSettings: %s\n",path.String()));

	BFile file(path.String(),B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);
	
	BMessage settings;

	currentset->ConvertToMessage(&settings);
	settings.Flatten(&file);
}

void APRView::LoadSettings(void)
{
	// Load the current GUI color settings from disk. This is done instead of
	// getting them from the server at this point for testing purposes.

/*	TODO: Uncomment the following disabled code when the app_server will handle the message

	// Query the server for the current settings
	port_id port=find_port(SERVER_PORT_NAME);
	if(port!=B_NAME_NOT_FOUND)
	{
		STRACE(("Retrieving settings from app_server\n"));
		PortLink link(port);
		PortMessage pmsg;
		
		link.SetOpCode(AS_GET_UI_COLORS);
		link.FlushWithReply(&pmsg);
		pmsg.Read<ColorSet>(currentset);
	}
	else
	{
*/
		STRACE(("Loading settings from disk\n"));
		
		BDirectory dir,newdir;
		if(dir.SetTo(SERVER_SETTINGS_DIR)==B_ENTRY_NOT_FOUND)
		{
			STRACE(("Color set folder not found. Creating %s\n",SERVER_SETTINGS_DIR));
			create_directory(SERVER_SETTINGS_DIR,0777);
		}
	
		BString path(SERVER_SETTINGS_DIR);
		path+=COLOR_SETTINGS_NAME;
		BFile file(path.String(),B_READ_ONLY);
	
		if(file.InitCheck()!=B_OK)
		{
			STRACE(("Couldn't open file %s for read\n",path.String()));
			SetDefaults();
			SaveSettings();
			return;
		}
	
		BMessage settings;
		if(settings.Unflatten(&file)!=B_OK)
		{
			STRACE(("Error unflattening SystemColors file %s\n",path.String()));
			
			SetDefaults();
			SaveSettings();
		}

		currentset->ConvertFromMessage(&settings);
//	}

//	SetColorSetName(currentset->name.String());
	
	UpdateControlsFromAttr(attrstring.String());
	
	if(currentset->name.Compare("Default")!=0)
		defaults->SetEnabled(true);
}

void APRView::SetDefaults(void)
{
	STRACE(("Initializing color settings to defaults\n"));
	defaults->SetEnabled(false);
	currentset->name.SetTo("Default");

	currentset->SetToDefaults();
//	SetColorSetName("Default");
}

void APRView::NotifyServer(void)
{
	// Send a message to the app_server with the settings which we have.

	port_id port=find_port(SERVER_PORT_NAME);
	
	if(port!=B_NAME_NOT_FOUND)
	{
		STRACE(("Notifying app_server\n"));
		PortLink link(port);
		
		link.SetOpCode(AS_SET_UI_COLORS);
		link.Attach<ColorSet>(*currentset);
		link.Flush();
	}
	
	// We also need to send the message to the window so that the Decorators tab updates itself
	if(Window())
		Window()->PostMessage(SET_UI_COLORS);
}

void APRView::UpdateControlsFromAttr(const char *string)
{
	if(!string)
		return;
	STRACE(("Update color for %s\n",string));

	picker->SetValue(currentset->StringToColor(string).GetColor32());
	colorwell->SetColor(picker->ValueAsColor());
	colorwell->Invalidate();
}
