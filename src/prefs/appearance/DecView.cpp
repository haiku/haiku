/*
	DecView.cpp:	The Appearance app's decorator handler

	Utilizes app_server graphics module emulation to allows decorators to display
	a preview. Saves the chosen decorator's path in a settings file in the app_server
	settings directory.
*/

#include <OS.h>
#include <String.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include "DecView.h"
#include "PortLink.h"
#include <Directory.h>
#include <File.h>
#include <stdio.h>
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

	LayerData ldata;
	ldata.highcolor.SetColor(51,102,160);
	uint64 pat=0xFFFFFFFFFFFFFFFFLL;
	driver->FillRect(preview_bounds,&ldata,(int8*)&pat);

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

void DecView::AttachedToWindow(void)
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
			
			success=LoadDecorator( ConvertIndexToPath( declist->CurrentSelection() ) );

			if(!success)
				break;
			
			LayerData ldata;
			ldata.highcolor.SetColor(ui_color(B_DESKTOP_COLOR));
			uint64 pat=0xFFFFFFFFFFFFFFFFLL;
			driver->FillRect(preview_bounds,&ldata,(int8*)&pat);
			if(decorator)
				decorator->Draw();
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
printf("GetDecorators\n");
#endif
	BDirectory dir;
	BEntry entry;
	BPath path;
	BString name;
	BStringItem *item;

	if(dir.SetTo(DECORATORS_DIR)!=B_OK)
		return;

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
	get_decorator_version *pversionfunc=NULL;
	status_t stat;
	image_id addon;
	
	addon=load_add_on(path);

	stat=get_image_symbol(addon, "get_decorator_version", B_SYMBOL_TYPE_TEXT, (void**)&pversionfunc);
	if(stat!=B_OK)
	{
		unload_add_on(addon);
#ifdef DEBUG_DECORATOR
printf("LoadDecorator(%s): Couldn't get version symbol\n",path);
#endif
		return false;
	}

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
		unload_add_on(decorator_id);
	}
	decorator_id=addon;
	decorator=pcreatefunc(SRect(50,50,150,150),WLOOK_TITLED,WFEEL_NORMAL,0);
	decorator->SetDriver(driver);
	decorator->SetFocus(true);
//	decorator->Draw();
	return true;
}

const char * DecView::ConvertIndexToPath(int32 index)
{
	BStringItem *item=(BStringItem*)declist->ItemAt(index);
	if(!item)
		return NULL;
	BString path(DECORATORS_DIR);
	path+=item->Text();
	return path.String();
}
