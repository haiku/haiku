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
#include <String.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include "DecView.h"
#include <PortLink.h>
#include <Directory.h>
#include <File.h>
#include <stdio.h>
#include "RGBColor.h"
#include "defs.h"
#include "PreviewDriver.h"
#include "Decorator.h"

#define DEBUG_DECORATOR

DecView::DecView(BRect frame, const char *name, int32 resize, int32 flags)
	:BView(frame,name,resize,flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	// Set up list of color attributes
	declist=new BListView(BRect(10,10,110,110),"DecoratorList");
	
	scrollview=new BScrollView("ScrollView",declist, B_FOLLOW_LEFT |
		B_FOLLOW_TOP, 0, false, true);
	AddChild(scrollview);
	scrollview->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	declist->SetSelectionMessage(new BMessage(DECORATOR_CHOSEN));

	BRect cvrect(0,0,50,25);

	cvrect.OffsetTo(Bounds().right-60,
		scrollview->Frame().top);

	apply=new BButton(cvrect,"ApplyButton","Apply",
		new BMessage(APPLY_SETTINGS),B_FOLLOW_LEFT |B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE);
	AddChild(apply);

	// set up app_server emulation
	
	driver=new PreviewDriver();
	if(!driver->Initialize())
		printf("Uh-oh... Couldn't initialize graphics module for server emu!\n");
	else
	{
		preview=driver->View();
		AddChild(preview);
		preview->MoveTo(scrollview->Frame().right+20,scrollview->Frame().top);
	}

	ldata.highcolor.SetColor(51,102,160);
	pat_solid_high=0xFFFFFFFFFFFFFFFFLL;
	driver->FillRect(preview_bounds,&ldata,(int8*)&pat_solid_high);

	decorator=NULL;
	decorator_id=-1;
	GetDecorators();	
	LoadSettings();
}

DecView::~DecView(void)
{
	driver->Shutdown();
	delete driver;
	if(decorator!=NULL)
		delete decorator;
}

void DecView::AllAttached(void)
{
	declist->SetTarget(this);
	apply->SetTarget(this);
	declist->Select(0);
}

void DecView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case APPLY_SETTINGS:
		{
			SaveSettings();
			NotifyServer();
			break;
		}
		case DECORATOR_CHOSEN:
		{
#ifdef DEBUG_DECORATOR
printf("MSG: Decorator Chosen - #%ld\n",declist->CurrentSelection());
#endif
			bool success=false;

			BString path( ConvertIndexToPath(declist->CurrentSelection()) );
#ifdef DEBUG_DECORATOR
printf("MSG: Decorator path: %s\n",path.String());
#endif

			success=LoadDecorator(path.String());

			if(!success)
			{
#ifdef DEBUG_DECORATOR
printf("MSG: Decorator NOT Chosen - couldn't load decorator\n");
#endif
				break;
			}
			
//			ldata.highcolor.SetColor(colorset.desktop);
			driver->FillRect(preview_bounds,&ldata,(int8*)&pat_solid_high);
			if(decorator)
			{
				decorator->SetColors(colorset);
				decorator->Draw();
			}
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}

void DecView::SaveSettings(void)
{
	BString path(SETTINGS_DIR);
	path+="DecoratorSettings";
	printf("%s\n",path.String());
	BFile file(path.String(),B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);
	
	settings.Flatten(&file);
}

void DecView::LoadSettings(void)
{
	settings.MakeEmpty();

	BDirectory dir,newdir;
	if(dir.SetTo(SETTINGS_DIR)==B_ENTRY_NOT_FOUND)
		create_directory(SETTINGS_DIR,0777);

	BString path(SETTINGS_DIR);
	path+="DecoratorSettings";
	BFile file(path.String(),B_READ_ONLY);

	if(file.InitCheck()==B_OK)
	{
		if(settings.Unflatten(&file)==B_OK)
		{
			// Get the color for the current attribute here
			return;
		}
		printf("Error unflattening settings file %s\n",path.String());
	}
	
	// If we get this far, we have encountered an error, so reset the settings
	// to the defaults
	SetDefaults();
}

void DecView::SetDefaults(void)
{
	settings.MakeEmpty();

	// Add default settings here
}

void DecView::NotifyServer(void)
{
	// Send a message to the app_server with the settings which we have.
	// While it might be worthwhile to someday have a separate protocol for
	// updating just one color, for now, we just need something to get things
	// done. Extract all the colors from the settings BMessage, attach them
	// to a PortLink message and fire it off to the server.
	
	// Name taken from ServerProtocol.h
	port_id serverport=find_port("OBappserver");

	if(serverport==B_NAME_NOT_FOUND)
		return;
	
	PortLink *pl=new PortLink(serverport);

//	pl->SetOpCode(SET_UI_COLORS);
	
	// extract settings from settings object and attach to portlink object

//	pl->Flush();
	delete pl;
}

void DecView::GetDecorators(void)
{
#ifdef DEBUG_DECORATOR
printf("DecView::GetDecorators()\n");
#endif
	BDirectory dir;
	BEntry entry;
	BPath path;
	BString name;
	BStringItem *item;

	status_t dirstat=dir.SetTo(DECORATORS_DIR);
	if(dirstat!=B_OK)
		if(dirstat!=B_OK)
	{
		// We couldn't set the directory, so more than likely it just
		// doesn't exist. Create it and return an empty menu.
		switch(dirstat)
		{
			case B_NAME_TOO_LONG:
			{
				printf("DecView::GetDecorators: Couldn't open the folder for decorators - path string too long.\n");
				break;
			}
			case B_ENTRY_NOT_FOUND:
			{
				printf("Couldn't open the folder for decorators - entry not found\n");
				break;
			}
			case B_BAD_VALUE:
			{
				printf("DecView::GetDecorators: Couldn't open the folder for decorators - bad path\n");
				break;
			}
			case B_NO_MEMORY:
			{
				printf("DecView::GetDecorators: No memory left. We're probably going to crash now\n");
				break;
			}
			case B_BUSY:
			{
				printf("DecView::GetDecorators: Couldn't open the folder for decorators - node busy\n");
				break;
			}
			case B_FILE_ERROR:
			{
				printf("DecView::GetDecorators: Couldn't open the folder for decorators - general file error\n");
				break;
			}
			case B_NO_MORE_FDS:
			{
				printf("DecView::GetDecorators: Couldn't open the folder for decorators - no more file descriptors\n");
				break;
			}
		}
		return;
	}

	int32 count=dir.CountEntries();

	for(int32 i=0;i<count;i++)
	{
		dir.GetNextEntry(&entry);
		entry.GetPath(&path);
		name=path.Path();
		name.Remove(0,name.FindLast('/')+1);

		item=new BStringItem(name.String());
		declist->AddItem(item);
	}
	
}

bool DecView::LoadDecorator(const char *path)
{
	if(!path)
		return false;
	create_decorator *pcreatefunc=NULL;
//	get_decorator_version *pversionfunc=NULL;
	status_t stat;
	image_id addon;
	
	addon=load_add_on(path);

/*	stat=get_image_symbol(addon, "get_decorator_version", B_SYMBOL_TYPE_TEXT, (void**)&pversionfunc);
	if(stat!=B_OK)
	{
		unload_add_on(addon);
#ifdef DEBUG_DECORATOR
printf("LoadDecorator(%s): Couldn't get version symbol\n",path);
#endif
		return false;
	}
*/
	// As of now, we do nothing with decorator versions, but the possibility exists
	// that the API will change even though I cannot forsee any reason to do so. If
	// we *did* do anything with decorator versions, the assignment to a global would
	// go here.
		
	stat=get_image_symbol(addon, "instantiate_decorator", B_SYMBOL_TYPE_TEXT, (void**)&pcreatefunc);
	if(stat!=B_OK)
	{
		unload_add_on(addon);
#ifdef DEBUG_DECORATOR
printf("LoadDecorator(%s): Couldn't get allocation symbol\n",path);
#endif
		return false;
	}
	if(decorator!=NULL)
	{
#ifdef DEBUG_DECORATOR
printf("LoadDecorator(): Deleting old decorator\n");
#endif
		delete decorator;
		decorator=NULL;

		// NEVER, *EVER*, call unload_add_on() if there is _any_ possibility,
		// no matter remote, that memory might still be in use when it is called.
		// It wll lead to unpredictable crashes which have almost nothing to
		// do with the *real* cause.
//		unload_add_on(decorator_id);
	}
	decorator_id=addon;
	decorator=pcreatefunc(BRect(50,50,150,150),B_TITLED_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,0);
	decorator->SetDriver(driver);
	decorator->SetFocus(true);
//	decorator->Draw();
	return true;
}

BString DecView::ConvertIndexToPath(int32 index)
{
	BStringItem *item=(BStringItem*)declist->ItemAt(index);
	if(!item)
	{
#ifdef DEBUG_DECORATOR
printf("ConvertIndexToPath(): Couldn't get item for index %ld\n",index);
#endif
		return NULL;
	}
	BString path(DECORATORS_DIR);
	path+=item->Text();
#ifdef DEBUG_DECORATOR
printf("ConvertIndexToPath(): returned %s\n",path.String());
#endif
	return BString(path.String());
}

void DecView::SetColors(const BMessage &message)
{
#ifdef DEBUG_DECORATOR
printf("DecView::SetColors\n");
#endif
	if(UnpackSettings(&colorset,&message))
	{
		if(decorator)
		{
//			ldata.highcolor.SetColor(colorset.desktop);
			driver->FillRect(preview_bounds,&ldata,(int8*)&pat_solid_high);
			decorator->SetColors(colorset);
			decorator->Draw();
		}
		else
		{
#ifdef DEBUG_DECORATOR
printf("DecView::SetColors: NULL decorator\n");
#endif
		}
	}
	else
	{
#ifdef DEBUG_DECORATOR
printf("DecView::SetColors: UnpackSetting returned false\n");
#endif
	}
}

bool DecView::UnpackSettings(ColorSet *set, const BMessage *msg)
{
	if(!set || !msg)
	{
#ifdef DEBUG_DECORATOR
printf("UnpackSettings(): NULL parameter\n");
#endif
		return false;
	}
	
	rgb_color *col;
	ssize_t size;

	// Once the OBOS app_server is in place, there will be more attributes

	if(msg->FindData("PANEL_BACKGROUND",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->panel_background.SetColor(*col);
	if(msg->FindData("MENU_BACKGROUND",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->menu_background.SetColor(*col);
	if(msg->FindData("MENU_SELECTION_BACKGROUND",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->menu_selected_background.SetColor(*col);
	if(msg->FindData("MENU_ITEM_TEXT",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->menu_text.SetColor(*col);
	if(msg->FindData("MENU_SELECTED_ITEM_TEXT",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->menu_selected_text.SetColor(*col);
	if(msg->FindData("WINDOW_TAB",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->window_tab.SetColor(*col);
	if(msg->FindData("KEYBOARD_NAVIGATION_BASE",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->keyboard_navigation_base.SetColor(*col);
	if(msg->FindData("KEYBOARD_NAVIGATION_PULSE",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->keyboard_navigation_pulse.SetColor(*col);
	return true;
}
