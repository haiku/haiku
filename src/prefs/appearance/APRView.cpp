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
#include "defs.h"
#include "ColorWell.h"
#include <ColorUtils.h>
#include <InterfaceDefs.h>
#include "ColorWhichItem.h"

//#define DEBUG_COLORSET

// uncomment this line to query the server for the current GUI settings
#define LOAD_SETTINGS_FROM_DISK

#define SAVE_COLORSET 'svcs'
#define DELETE_COLORSET 'dlcs'
#define LOAD_COLORSET 'ldcs'
#define COLOR_DROPPED 'cldp'

APRView::APRView(const BRect &frame, const char *name, int32 resize, int32 flags)
	:BView(frame,name,resize,flags), settings(B_SIMPLE_DATA)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BMenuBar *mb=new BMenuBar(BRect(0,0,Bounds().Width(),16),"menubar");

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

	BRect wellrect(0,0,20,20);
	wellrect.OffsetTo(Bounds().Width()-(wellrect.Width()+10),25);

	colorwell=new ColorWell(wellrect,new BMessage(COLOR_DROPPED));
	AddChild(colorwell);

	wellrect.OffsetTo(10,25);
	wellrect.right=colorwell->Frame().left - 20;
	colorset_label=new BStringView(wellrect,"colorset_label","Color Set: ");
	AddChild(colorset_label);
	colorset_name=new BString("<untitled>");


	// Set up list of color attributes
	BRect rect(10,60,200,160);
	attrlist=new BListView(rect,"AttributeList");
	
	scrollview=new BScrollView("ScrollView",attrlist, B_FOLLOW_LEFT |
		B_FOLLOW_TOP, 0, false, true);
	AddChild(scrollview);
	scrollview->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	attrlist->SetSelectionMessage(new BMessage(ATTRIBUTE_CHOSEN));

	attrlist->AddItem(new ColorWhichItem(B_PANEL_BACKGROUND_COLOR));
#ifndef BUILD_UNDER_R5
	attrlist->AddItem(new ColorWhichItem(B_PANEL_TEXT_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_DOCUMENT_BACKGROUND_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_DOCUMENT_TEXT_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_CONTROL_BACKGROUND_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_CONTROL_TEXT_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_CONTROL_BORDER_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_CONTROL_HIGHLIGHT_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_NAVIGATION_BASE_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_NAVIGATION_PULSE_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_SHINE_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_SHADOW_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_MENU_BACKGROUND_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_MENU_SELECTED_BACKGROUND_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_MENU_ITEM_TEXT_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_MENU_SELECTED_ITEM_TEXT_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_MENU_SELECTED_BORDER_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_TOOLTIP_BACKGROUND_COLOR));
	
	attrlist->AddItem(new ColorWhichItem(B_SUCCESS_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_FAILURE_COLOR));
#else
	attrlist->AddItem(new ColorWhichItem(B_MENU_BACKGROUND_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_MENU_SELECTION_BACKGROUND_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_MENU_ITEM_TEXT_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_MENU_SELECTED_ITEM_TEXT_COLOR));
	attrlist->AddItem(new ColorWhichItem(B_KEYBOARD_NAVIGATION_COLOR));
#endif	
	attrlist->AddItem(new ColorWhichItem(B_WINDOW_TAB_COLOR));

	picker=new BColorControl(BPoint(scrollview->Frame().right+20,scrollview->Frame().top),B_CELLS_32x8,5.0,"Picker",
		new BMessage(UPDATE_COLOR));
	AddChild(picker);
	
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
	// Eventually we might want to have this disabled at the start and
	// enabled when the user makes changes and such. Then again, maybe we won't.
//	revert->SetEnabled(false);
	
	cvrect.OffsetBy(70,0);
	try_settings=new BButton(cvrect,"TryButton","Try",
		new BMessage(TRY_SETTINGS),B_FOLLOW_LEFT |B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE);
	AddChild(try_settings);
	
	// Eventually we might want to have this disabled at the start and
	// enabled when the user makes changes and such. Then again, maybe we won't.
//	try_settings->SetEnabled(false);
	
	cvrect.OffsetBy(70,0);
	apply=new BButton(cvrect,"ApplyButton","Apply",
		new BMessage(APPLY_SETTINGS),B_FOLLOW_LEFT |B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE);
	AddChild(apply);

	BEntry entry(COLOR_SET_DIR);
	entry_ref ref;
	entry.GetRef(&ref);
	savepanel=new BFilePanel(B_SAVE_PANEL, NULL,
		&ref, 0, false);

	attribute=B_PANEL_BACKGROUND_COLOR;
	attrstring="PANEL_BACKGROUND";
	LoadSettings();
	picker->SetValue(GetColorFromMessage(&settings,attrstring.String()));
}

APRView::~APRView(void)
{
	delete savepanel;
	delete colorset_name;
}

void APRView::AllAttached(void)
{
	picker->SetTarget(this);
	attrlist->Select(0);
	attrlist->SetTarget(this);
	apply->SetTarget(this);
	defaults->SetTarget(this);
	try_settings->SetTarget(this);
	revert->SetTarget(this);
	settings_menu->SetTargetForItems(this);
	colorset_menu->SetTargetForItems(this);
	colorwell->SetTarget(this);

	BMessenger msgr(this);
	savepanel->SetTarget(msgr);
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
		}
	}

	switch(msg->what)
	{
		case B_WORKSPACE_ACTIVATED:
		{
			BScreen screen;
			rgb_color col=screen.DesktopColor();
			settings.ReplaceData("DESKTOP",(type_code)'RGBC',
					&col,sizeof(rgb_color));
			
			if(attrstring=="DESKTOP")
			{
				picker->SetValue(col);
				colorwell->SetColor(col);
			}
			break;
		}
		case DELETE_COLORSET:
		{
			// Construct the path and delete
			BString path(COLOR_SET_DIR);
			path+=*colorset_name;
			
			BString printstring("Remove ");
			printstring+=*colorset_name;
			printstring+=" from disk permenantly?";
			BAlert *a=new BAlert("OpenBeOS",printstring.String(),"Yes", "No");
			if(a->Go()==0)
			{
				int stat=remove(path.String());
				if(stat!=0)
				{
#ifdef DEBUG_COLORSET
printf("MSG: Delete Request - couldn't delete file %s\n",path.String());
#endif
				}
				else
				{
					BMenuItem *item=colorset_menu->FindItem(colorset_name->String());
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
#ifdef DEBUG_COLORSET
printf("MSG: Load Request - couldn't find file name\n");
#endif
				break;
			}
			LoadColorSet(name);
			break;
		}
		case SAVE_COLORSET:
		{
			savepanel->Show();
			break;
		}
		case B_SAVE_REQUESTED:
		{
			BString name;
			if(msg->FindString("name",&name)!=B_OK)
			{
#ifdef DEBUG_COLORSET
printf("MSG: Save Request - couldn't find file name\n");
#endif
				break;
			}
			SaveColorSet(name);
			break;
		}
		case UPDATE_COLOR:
		{
			// Received from the color picker when its color changes

			rgb_color col=picker->ValueAsColor();

			colorwell->SetColor(col);
			colorwell->Invalidate();
			
			// Update current attribute in the settings
			settings.ReplaceData(attrstring.String(),(type_code)'RGBC',
			&col,sizeof(rgb_color));

			SetColorSetName("<untitled>");
			if(Window())
				Window()->PostMessage(SET_UI_COLORS);
			break;
		}
		case ATTRIBUTE_CHOSEN:
		{
			// Received when the user chooses a GUI attribute from the list
			ColorWhichItem *whichitem=(ColorWhichItem*)attrlist->ItemAt(attrlist->CurrentSelection());
			if(!whichitem)
				break;
			
			rgb_color col=GetColorFromMessage(&settings,whichitem->Text(),0);
			picker->SetValue(col);
			colorwell->SetColor(col);
			colorwell->Invalidate();

//			SetColorSetName("<untitled>");
//			if(Window())
//				Window()->PostMessage(SET_UI_COLORS);
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
			LoadSettings();
			rgb_color col=GetColorFromMessage(&settings,attrstring.String(),0);
			picker->SetValue(col);
			colorwell->SetColor(col);
			colorwell->Invalidate();
			if(Window())
				Window()->PostMessage(SET_UI_COLORS);
			break;
		}
		case DEFAULT_SETTINGS:
		{
			SetDefaults();
			rgb_color col=GetColorFromMessage(&settings,attrstring.String(),0);
			picker->SetValue(col);
			colorwell->SetColor(col);
			colorwell->Invalidate();
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
#ifdef DEBUG_COLORSET
printf("Loading color sets from disk\n");
#endif
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
				printf("APRView::LoadColorSets(): Invalid colorset folder path.\n");
				break;
			}
			case B_NO_MEMORY:
			{
				printf("APRView::LoadColorSets(): No memory left. We're probably going to crash now. \n");
				break;
			}
			case B_BUSY:
			{
				printf("APRView::LoadColorSets(): Busy node " COLOR_SET_DIR "\n");
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

	int32 count=dir.CountEntries();

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
	
	return menu;
}	

void APRView::LoadColorSet(const BString &name)
{
	// Load the current GUI color settings from a color set file.

#ifdef DEBUG_COLORSET
printf("LoadColorSet: %s\n",name.String());
#endif

	BDirectory dir,newdir;
	if(dir.SetTo(COLOR_SET_DIR)==B_ENTRY_NOT_FOUND)
	{
#ifdef DEBUG_COLORSET
printf("Color set folder not found. Creating %s\n",COLOR_SET_DIR);
#endif
		create_directory(COLOR_SET_DIR,0777);
	}

	BString path(COLOR_SET_DIR);
	path+=name.String();
	BFile file(path.String(),B_READ_ONLY);

	if(file.InitCheck()!=B_OK)
	{
#ifdef DEBUG_COLORSET
printf("Couldn't open file %s for read\n",path.String());
#endif
		return;
	}
	if(settings.Unflatten(&file)==B_OK)
	{
#ifdef DEBUG_COLORSET
settings.PrintToStream();
#endif
		BString internal_name;
		settings.FindString("name",&internal_name);
//		BString namestr("Color Set: ");
//		namestr+=internal_name.String();
//		colorset_label->SetText(namestr.String());
		SetColorSetName(internal_name.String());

		picker->SetValue(GetColorFromMessage(&settings,attrstring.String()));
		colorwell->SetColor(picker->ValueAsColor());
		return;
	}
#ifdef DEBUG_COLORSET
printf("Error unflattening file %s\n",name.String());
#endif
}

void APRView::SaveColorSet(const BString &name)
{

	// Save the current color attributes as a flattened BMessage in the 
	// color set folder
	BString path(COLOR_SET_DIR);
	path+=name.String();

#ifdef DEBUG_COLORSET
printf("SaveColorSet: %s\n",path.String());
#endif

	if(settings.ReplaceString("name",name.String())!=B_OK)
	{
#ifdef DEBUG_COLORSET
printf("SaveColorSet: Couldn't replace set name in settings\n");
#endif
	}

	BFile file(path.String(),B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);

	if(file.InitCheck()!=B_OK)
	{
#ifdef DEBUG_COLORSET
printf("SaveColorSet: Couldn't open settings file for write\n");
#endif
	}
	
	if(settings.Flatten(&file)!=B_OK)
	{
#ifdef DEBUG_COLORSET
printf("SaveColorSet: Couldn't flatten settings to file\n");
#endif
		return;
	}

	BMessage *msg=new BMessage(LOAD_COLORSET);
	msg->AddString("name",name.String());

	if(colorset_menu->AddItem(new BMenuItem(name.String(),msg))==false)
	{
#ifdef DEBUG_COLORSET
printf("SaveColorSet: Error in adding item to menu\n");
#endif
	}
	SetColorSetName(name.String());
}

void APRView::SetColorSetName(const char *name)
{
	if(!name)
		return;
	BString namestr("Color Set: ");
	colorset_name->SetTo(name);
	namestr+=name;
	colorset_label->SetText(namestr.String());
	colorset_label->Invalidate();
}

void APRView::SaveSettings(void)
{
	// Save the current GUI color settings to the GUI colors file in the 
	// path specified in defs.h
	
	BString path(SETTINGS_DIR);
	path+=COLOR_SETTINGS_NAME;
#ifdef DEBUG_COLORSET
printf("SaveSettings: %s\n",path.String());
#endif
	BFile file(path.String(),B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);
	
	settings.Flatten(&file);
}

void APRView::LoadSettings(void)
{
	// Load the current GUI color settings from disk. This is done instead of
	// getting them from the server at this point for testing purposes. Comment
	// out the #define LOAD_SETTINGS_FROM_DISK line to use the server query code

#ifdef LOAD_SETTINGS_FROM_DISK

#ifdef DEBUG_COLORSET
printf("Loading settings from disk\n");
#endif
	settings.MakeEmpty();

	BDirectory dir,newdir;
	if(dir.SetTo(SETTINGS_DIR)==B_ENTRY_NOT_FOUND)
	{
#ifdef DEBUG_COLORSET
printf("Color set folder not found. Creating %s\n",SETTINGS_DIR);
#endif
		create_directory(SETTINGS_DIR,0777);
	}

	BString path(SETTINGS_DIR);
	path+=COLOR_SETTINGS_NAME;
	BFile file(path.String(),B_READ_ONLY);

	if(file.InitCheck()!=B_OK)
	{
#ifdef DEBUG_COLORSET
printf("Couldn't open file %s for read\n",path.String());
#endif
		SetDefaults();
		SaveSettings();
		return;
	}
	if(settings.Unflatten(&file)==B_OK)
	{
		settings.FindString("name",colorset_name);
		SetColorSetName(colorset_name->String());

		picker->SetValue(GetColorFromMessage(&settings,attrstring.String()));
		colorwell->SetColor(picker->ValueAsColor());
#ifdef DEBUG_COLORSET
settings.PrintToStream();
#endif
		return;
	}
#ifdef DEBUG_COLORSET
printf("Error unflattening SystemColors file %s\n",path.String());
#endif
	
	// If we get this far, we have encountered an error, so reset the settings
	// to the defaults
	SetDefaults();
	SaveSettings();
#else
	// We will query the app_server via a bunch of ui_color() calls and then
	// add the appropriate colors to the message
	rgb_color col;
	ColorWhichItem whichitem(B_PANEL_BACKGROUND_COLOR);

	col=ui_color(whichitem.GetAttribute());
	settings.AddData("B_PANEL_BACKGROUND_COLOR",(type_code)'RGBC',&col,sizeof(rgb_color));

#ifndef BUILD_UNDER_R5
	whichitem.SetAttribute();
	col=ui_color(B_PANEL_TEXT_COLOR);
	whichitem.SetAttribute(B_PANEL_TEXT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_DOCUMENT_BACKGROUND_COLOR);
	whichitem.SetAttribute(B_DOCUMENT_BACKGROUND_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_DOCUMENT_TEXT_COLOR);
	whichitem.SetAttribute(B_DOCUMENT_TEXT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_CONTROL_BACKGROUND_COLOR);
	whichitem.SetAttribute(B_CONTROL_BACKGROUND_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_CONTROL_TEXT_COLOR);
	whichitem.SetAttribute(B_CONTROL_TEXT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_CONTROL_BORDER_COLOR);
	whichitem.SetAttribute(B_CONTROL_BORDER_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_CONTROL_HIGHLIGHT_COLOR);
	whichitem.SetAttribute(B_CONTROL_HIGHLIGHT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_NAVIGATION_BASE_COLOR);
	whichitem.SetAttribute(B_NAVIGATION_BASE_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_NAVIGATION_PULSE_COLOR);
	whichitem.SetAttribute(B_NAVIGATION_PULSE_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_SHINE_COLOR);
	whichitem.SetAttribute(B_SHINE_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_SHADOW_COLOR);
	whichitem.SetAttribute(B_SHADOW_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_MENU_SELECTED_BORDER_COLOR);
	whichitem.SetAttribute(B_MENU_SELECTED_BORDER_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_TOOLTIP_BACKGROUND_COLOR);
	whichitem.SetAttribute(B_TOOLTIP_BACKGROUND_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_TOOLTIP_TEXT_COLOR);
	whichitem.SetAttribute(B_TOOLTIP_TEXT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_SUCCESS_COLOR);
	whichitem.SetAttribute(B_SUCCESS_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_FAILURE_COLOR);
	whichitem.SetAttribute(B_FAILURE_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
#endif
	col=ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR);
	whichitem.SetAttribute(B_MENU_SELECTED_ITEM_TEXT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_MENU_BACKGROUND_COLOR);
	whichitem.SetAttribute(B_MENU_BACKGROUND_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_MENU_SELECTION_BACKGROUND_COLOR);
	whichitem.SetAttribute(B_MENU_SELECTION_BACKGROUND_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_MENU_ITEM_TEXT_COLOR);
	whichitem.SetAttribute(B_MENU_ITEM_TEXT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
	col=ui_color(B_WINDOW_TAB_COLOR);
	whichitem.SetAttribute(B_WINDOW_TAB_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	
#endif
}

void APRView::SetDefaults(void)
{
#ifdef DEBUG_COLORSET
printf("Initializing color settings to defaults\n");
#endif
	settings.MakeEmpty();
	settings.AddString("name","Default");
	colorset_name->SetTo("Default");

	ColorWhichItem whichitem(B_PANEL_BACKGROUND_COLOR);
	rgb_color col={216,216,216,255};

#ifndef BUILD_UNDER_R5
	SetRGBColor(&col,0,0,0);
	whichitem.SetAttribute(B_PANEL_TEXT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,255,255,255);
	whichitem.SetAttribute(B_DOCUMENT_BACKGROUND_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,0);
	whichitem.SetAttribute(B_DOCUMENT_TEXT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,245,245,245);
	whichitem.SetAttribute(B_CONTROL_BACKGROUND_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,0);
	whichitem.SetAttribute(B_CONTROL_TEXT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,0);
	whichitem.SetAttribute(B_CONTROL_BORDER_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,115,120,184);
	whichitem.SetAttribute(B_CONTROL_HIGHLIGHT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,170,50,184);
	whichitem.SetAttribute(B_NAVIGATION_BASE_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,0);
	whichitem.SetAttribute(B_NAVIGATION_PULSE_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,255,255,255);
	whichitem.SetAttribute(B_SHINE_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,0);
	whichitem.SetAttribute(B_SHADOW_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,0);
	whichitem.SetAttribute(B_MENU_SELECTED_BORDER_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,255,255,0);
	whichitem.SetAttribute(B_TOOLTIP_BACKGROUND_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,0);
	whichitem.SetAttribute(B_TOOLTIP_TEXT_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,255,0);
	whichitem.SetAttribute(B_SUCCESS_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,255,0,0);
	whichitem.SetAttribute(B_FAILURE_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,51,102,160);
	whichitem.SetAttribute(B_MENU_SELECTED_BACKGROUND_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));
#else
	SetRGBColor(&col,51,102,160);
	whichitem.SetAttribute(B_MENU_SELECTION_BACKGROUND_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,51,102,160);
	whichitem.SetAttribute(B_KEYBOARD_NAVIGATION_COLOR);
	settings.AddData(whichitem.Text(),(type_code)'RGBC',&col,sizeof(rgb_color));

#endif

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

//	BString labelstr("Color Set: ");
//	labelstr+=colorset_name->String();
//	colorset_label->SetText(labelstr.String());
	SetColorSetName("Default");
}

void APRView::NotifyServer(void)
{
	// Send a message to the app_server with the settings which we have.
	// While it might be worthwhile to someday have a separate protocol for
	// updating just one color, for now, we just need something to get things
	// done. Extract all the colors from the settings BMessage, attach them
	// to a PortLink message and fire it off to the server.
	
	// For now, we do not have the OBOS app_server, so we simply do what
	// we can with tbe R5 server

	rgb_color col;

	// Set menu color
	menu_info minfo;
	get_menu_info(&minfo);
	col=GetColorFromMessage(&settings,"MENU_BACKGROUND");
	if(col.alpha==0 && col.red==0 && col.green==0 && col.blue==0)
	{
		// do nothing
	}
	else
	{
		minfo.background_color=col;
		set_menu_info(&minfo);
	}
	
	// Set desktop color
	BScreen screen;
	col=GetColorFromMessage(&settings,"DESKTOP");

#ifdef DEBUG_COLORSET
printf("NotifyServer: Setting Desktop color to "); PrintRGBColor(col);
#endif
	if(col.alpha==0 && col.red==0 && col.green==0 && col.blue==0)
	{
		// do nothing
	}
	else
		screen.SetDesktopColor(col);

	if(Window())
		Window()->PostMessage(SET_UI_COLORS);
	
/*	// Name taken from ServerProtocol.h
	port_id serverport=find_port("OBappserver");

	if(serverport==B_NAME_NOT_FOUND)
		return;
	
	PortLink *pl=new PortLink(serverport);

	// 'suic'==SET_UI_COLORS message from ServerProtocol.h
	pl->SetOpCode('suic');
	
	rgb_color col;
	col=GetColorFromMessage(&settings,"PANEL_BACKGROUND");
	pl->Attach(&col,sizeof(rgb_color));
	col=GetColorFromMessage(&settings,"MENU_BACKGROUND");
	pl->Attach(&col,sizeof(rgb_color));
	col=GetColorFromMessage(&settings,"MENU_SELECTION_BACKGROUND");
	pl->Attach(&col,sizeof(rgb_color));
	col=GetColorFromMessage(&settings,"MENU_ITEM_TEXT");
	pl->Attach(&col,sizeof(rgb_color));
	col=GetColorFromMessage(&settings,"MENU_SELECTED_ITEM_TEXT");
	pl->Attach(&col,sizeof(rgb_color));
	col=GetColorFromMessage(&settings,"WINDOW_TAB");
	pl->Attach(&col,sizeof(rgb_color));
	col=GetColorFromMessage(&settings,"KEYBOARD_NAVIGATION");
	pl->Attach(&col,sizeof(rgb_color));
	col=GetColorFromMessage(&settings,"DESKTOP");
	pl->Attach(&col,sizeof(rgb_color));

	pl->Flush();
	delete pl;
*/

}

rgb_color APRView::GetColorFromMessage(BMessage *msg, const char *name, int32 index=0)
{
	
	// Simple function to do the dirty work of getting an rgb_color from
	// a message
	rgb_color *col,rcolor={0,0,0,0};
	ssize_t size;

	if(!msg || !name)
		return rcolor;

	if(msg->FindData(name,(type_code)'RGBC',index,(const void**)&col,&size)==B_OK)
		rcolor=*col;
	return rcolor;
}
