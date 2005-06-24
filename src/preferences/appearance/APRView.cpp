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
#include <stdio.h>

#include <PortLink.h>
#include <InterfaceDefs.h>
#include <ServerProtocol.h>

#include "APRView.h"
#include "defs.h"
#include "ColorWell.h"
#include <ColorUtils.h>
#include "ColorWhichItem.h"
#include "ColorSet.h"

#include "ServerConfig.h"

//#define DEBUG_COLORSET

#ifdef DEBUG_COLORSET
#define STRACE(a) printf a
#else
#define STRACE(A) /* nothing */
#endif

#define COLOR_DROPPED 'cldp'

// Declarations for private functions in InterfaceDefs.cpp
namespace BPrivate
{
	void get_system_colors(ColorSet *colors);
	void set_system_colors(const ColorSet &colors);
}

APRView::APRView(const BRect &frame, const char *name, int32 resize, int32 flags)
	:BView(frame,name,resize,flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// Set up list of color attributes
	BRect rect(10,10,200,150);
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


	BRect wellrect(0,0,50,50);
	wellrect.OffsetTo( rect.right + 42, rect.top +
					(scrollview->Bounds().Height() - wellrect.Height())/2 );

	colorwell=new ColorWell(wellrect,new BMessage(COLOR_DROPPED),true);
	AddChild(colorwell);
	
	// Center the list and color well
	
	rect = scrollview->Frame();
	rect.right = wellrect.right;
	rect.OffsetTo( (Bounds().Width()-rect.Width())/2,rect.top);
	
	scrollview->MoveTo(rect.left, rect.top);
	colorwell->MoveTo(rect.right - colorwell->Bounds().Width(), colorwell->Frame().top);
	
	picker=new BColorControl(BPoint(10,scrollview->Frame().bottom+20),B_CELLS_32x8,5.0,"Picker",
		new BMessage(UPDATE_COLOR));
	AddChild(picker);
	
	picker->MoveTo( (Bounds().Width() - picker->Frame().Width())/2,
			scrollview->Frame().bottom + 20 );
	
	BRect cvrect;
	
	cvrect=picker->Frame();
	cvrect.right=cvrect.left+60;
	cvrect.bottom=cvrect.top+30;
	
	// 40 is a 10 pixel space between buttons and other objects, 
	// such as the edge or another button
	cvrect.OffsetTo( (Bounds().Width()- ((cvrect.Width()*3) + 40))/2,
					picker->Frame().bottom+10);

	defaults=new BButton(cvrect,"DefaultsButton","Defaults",
						new BMessage(DEFAULT_SETTINGS),
						B_FOLLOW_LEFT |B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	AddChild(defaults);

	cvrect.OffsetBy(70,0);
	revert=new BButton(cvrect,"RevertButton","Revert", 
						new BMessage(REVERT_SETTINGS),
						B_FOLLOW_LEFT |B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	AddChild(revert);
	revert->SetEnabled(false);
	
	cvrect.OffsetBy(70,0);
	apply=new BButton(cvrect,"ApplyButton","Apply", new BMessage(APPLY_SETTINGS),
						B_FOLLOW_LEFT |B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	AddChild(apply);
	apply->SetEnabled(false);

	BEntry entry(COLOR_SET_DIR);
	entry_ref ref;
	entry.GetRef(&ref);

	attribute=B_PANEL_BACKGROUND_COLOR;
	attrstring="Panel Background";
	
	LoadSettings();
	
	// Save values for when we need to revert
	prevset=currentset;
}

void APRView::AttachedToWindow(void)
{
	picker->SetTarget(this);
	attrlist->Select(0);
	attrlist->SetTarget(this);
	apply->SetTarget(this);
	defaults->SetTarget(this);
	revert->SetTarget(this);
	colorwell->SetTarget(this);

	picker->SetValue(currentset.StringToColor(attrstring.String()));
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
		case UPDATE_COLOR:
		{
			// Received from the color picker when its color changes
			
			rgb_color col=picker->ValueAsColor();

			colorwell->SetColor(col);
			colorwell->Invalidate();
			
			// Update current attribute in the settings
			currentset.SetColor(attrstring.String(),col);
			
			apply->SetEnabled(true);
			revert->SetEnabled(true);
			
			break;
		}
		case ATTRIBUTE_CHOSEN:
		{
			// Received when the user chooses a GUI attribute from the list
			
			ColorWhichItem *whichitem = (ColorWhichItem*)attrlist->ItemAt( attrlist->CurrentSelection() );
			
			if(!whichitem)
				break;
			
			attrstring=whichitem->Text();
			UpdateControlsFromAttr(whichitem->Text());
			break;
		}
		case APPLY_SETTINGS:
		{
			apply->SetEnabled(false);
			BPrivate::set_system_colors(currentset);
			
			BString path(SERVER_SETTINGS_DIR);
			path += COLOR_SETTINGS_NAME;
			
			SaveColorSet( path.String(), currentset);
			break;
		}
		case REVERT_SETTINGS:
		{
			currentset=prevset;
			UpdateControlsFromAttr(attrstring.String());
			
			// Save settings on disk if 'Apply' has been used
			if(!apply->IsEnabled())
			{
				BString path(SERVER_SETTINGS_DIR);
				path += COLOR_SETTINGS_NAME;
				SaveColorSet( path.String(), currentset);
			}
			apply->SetEnabled(false);
			revert->SetEnabled(false);
			
			break;
		}
		case DEFAULT_SETTINGS:
		{
			currentset.SetToDefaults();
			apply->SetEnabled(true);
			
			UpdateControlsFromAttr(attrstring.String());
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}

void APRView::LoadSettings(void)
{
	// Load the current GUI color settings from disk. This is done instead of
	// getting them from the server at this point for testing purposes.
	
	// Query the server for the current settings
	//BPrivate::get_system_colors(&currentset);
	
	// TODO: remove this and enable the get_system_colors() call
	LoadColorSet("/boot/home/config/settings/app_server/system_colors",&currentset);
		
	UpdateControlsFromAttr(attrstring.String());
}

void APRView::UpdateControlsFromAttr(const char *string)
{
	if(!string)
		return;
	STRACE(("Update color for %s\n",string));

	picker->SetValue(currentset.StringToColor(string));
	colorwell->SetColor(picker->ValueAsColor());
	colorwell->Invalidate();
}
