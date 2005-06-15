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
//	File Name:		DecView.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	decorator handler for the app
//  
//	Utilizes app_server graphics module emulation to allows decorators to display
//	a preview. Saves the chosen decorator's path in a settings file in the app_server
//	settings directory.
//------------------------------------------------------------------------------
#include <OS.h>
#include <Message.h>
#include <File.h>
#include <ScrollView.h>
#include <ListItem.h>
#include <stdio.h>
#include <Directory.h>
#include <Bitmap.h>

#include "ServerConfig.h"
#include "DecView.h"
#include "defs.h"

//#define DEBUG_DECORATOR

#ifdef DEBUG_DECORATOR
#define STRACE(a) printf a
#else
#define STRACE(a) /* nothing */
#endif

namespace BPrivate
{
	int32 count_decorators(void);
	status_t set_decorator(const int32 &index);
	int32 get_decorator(void);
	status_t get_decorator_name(const int32 &index, BString &name);
	status_t get_decorator_preview(const int32 &index, BBitmap *bitmap);
}

class DecoratorItem : public BStringItem
{
public:
			DecoratorItem(const char *text, const int32 &index);
	int32	GetIndex(void) const { return fIndex; }
	
private:
	int32	fIndex;
	
};

DecoratorItem::DecoratorItem(const char *text, const int32 &index)
 : 	BStringItem(text),
 	fIndex(index)
{
}

DecView::DecView(BRect frame, const char *name, int32 resize, int32 flags)
	:BView(frame,name,resize,flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BRect r;
	
	r.left = r.top = 10;
	
	r.right = Bounds().Width()/2 - 10;
	r.bottom = Bounds().bottom * .75;
	
	
	// TODO: This will eventually be just a BView which draws a BBitmap, but for now
	// we will use a placeholder
	preview = new BView(r, "preview", B_FOLLOW_NONE, B_WILL_DRAW);
	preview->SetViewColor(100,140,255);
	AddChild(preview);
	
	r.left = r.right + 20;
	r.right = Bounds().right - 10 - B_V_SCROLL_BAR_WIDTH;
	
	
	declist = new BListView(r, "listview");
	declist->SetSelectionMessage( new BMessage(DECORATOR_CHOSEN) );
	
	BScrollView *scrollView = new BScrollView("scrollview", declist, B_FOLLOW_NONE,
											0,false,true);
	AddChild(scrollView);
	
	apply = new BButton( BRect(0,0,1,1), "apply", "Apply", new BMessage(APPLY_SETTINGS),
						B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	apply->ResizeToPreferred();
	apply->MoveTo( Bounds().Width() - 10 - apply->Bounds().Width(),
					Bounds().Height() - 10 - apply->Bounds().Height() );
	AddChild(apply);
	
	// Now that we have all the controls in place, populate the list
	int32 decCount = BPrivate::count_decorators();
	
	for(int32 i=0; i<decCount; i++)
	{
		BString string;
		
		if( BPrivate::get_decorator_name( i, string ) == B_OK )
			declist->AddItem( new DecoratorItem( string.String(), i ) );
	}
}

void DecView::AttachedToWindow(void)
{
	declist->SetTarget(this);
	apply->SetTarget(this);

	int32 currentDec = BPrivate::get_decorator();
	if(currentDec>=0)
		declist->Select(currentDec);
}

void DecView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case APPLY_SETTINGS:
		{
			SaveSettings();
			
			int32 selection = declist->CurrentSelection();
			if(selection >=0 )
				BPrivate::set_decorator(selection);
			
			break;
		}
		
		case DECORATOR_CHOSEN:
		{
			// TODO: update the preview here
			break;
		}
		
		default:
			BView::MessageReceived(msg);
			break;
	}
}

void DecView::SaveSettings(void)
{
	BStringItem *item=(DecoratorItem*)declist->ItemAt(declist->CurrentSelection());
	if(!item)
		return;
	
	BString path(SERVER_SETTINGS_DIR);
	path+="decorator_settings";
	STRACE(("%s\n",path.String()));
	BFile file(path.String(),B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);
	
	BMessage settings;
	settings.AddString("decorator",item->Text());
	settings.Flatten(&file);
}

void DecView::LoadSettings(void)
{
	BMessage settings;

	BDirectory dir,newdir;
	if(dir.SetTo(SERVER_SETTINGS_DIR)==B_ENTRY_NOT_FOUND)
		create_directory(SERVER_SETTINGS_DIR,0777);

	BString path(SERVER_SETTINGS_DIR);
	path+="decorator_settings";
	BFile file(path.String(),B_READ_ONLY);

	if(file.InitCheck()!=B_OK)
	{
		STRACE(("Error opening settings file %s\n",path.String()));
		return;
	}
		
	if(settings.Unflatten(&file)!=B_OK)
	{
		STRACE(("Error unflattening settings file %s\n",path.String()));
		return;
	}
	
	BString itemtext;
	if(settings.FindString("decorator",&itemtext)!=B_OK)
	{
		STRACE(("Couldn't find decorator name in settings file %s\n",path.String()));
		return;
	}
	
	for(int32 i=0;i<declist->CountItems();i++)
	{
		BStringItem *item=(BStringItem*)declist->ItemAt(i);
		if(item && itemtext.Compare(item->Text())==0)
		{
			declist->Select(i);
			return;
		}
	}

}
