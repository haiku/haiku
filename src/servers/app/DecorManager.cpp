/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author: DarkWyrm <bpmagic@columbus.rr.com>
 */
 
#include <Directory.h>
#include <Rect.h>
#include <File.h>
#include <Message.h>
#include <Entry.h>

#include "ColorSet.h"
#include "DefaultDecorator.h"
#include "ServerConfig.h"
#include "DecorManager.h"

// Globals
DecorManager gDecorManager;

extern ColorSet gui_colorset;

Decorator* create_default_decorator(BRect rect, int32 wlook, int32 wfeel, 
									int32 wflags);

// This is a class used only by the DecorManager to track all the decorators in memory
class DecorInfo
{
public:
						DecorInfo(const image_id &id, const char *name,
									create_decorator *alloc);
						~DecorInfo(void);
	image_id			GetID(void) const { return fID; }
	const char *		GetName(void) const { return fName.String(); }
	Decorator *			Instantiate(BRect rect, const char *title, 
									int32 wlook, int32 wfeel,
									int32 wflags, DisplayDriver *ddriver);
private:

	image_id			fID;
	BString 			fName;
	create_decorator	*fAllocator;
};

DecorInfo::DecorInfo(const image_id &id, const char *name, create_decorator *alloc)
 :	fID(id),
 	fName(name),
 	fAllocator(alloc)
{
}

DecorInfo::~DecorInfo(void)
{
	// Do nothing. Normal programming practice would say that one should unload
	// the object's associate image_id. However, there is some  funkiness with
	// the R5 kernel in which addons aren't unloaded when unload_add_on() is
	// called -- perhaps it's lazy unloading or something. In any event, it
	// causes crashes which are *extremely* hard to track down to this.
	
	// Considering the usage of DecorInfo and DecorManager, we can live with
	// this because the app_server will not the DecorManager is freed only
	// when the app_server quits. It's not pretty, but it gets the job done.
}

Decorator *
DecorInfo::Instantiate(BRect rect, const char *title, int32 wlook, int32 wfeel,
							int32 wflags, DisplayDriver *ddriver)
{
	Decorator *dec = fAllocator(rect, wlook, wfeel, wflags);

	dec->SetDriver(ddriver);
	
	gui_colorset.Lock();
	dec->SetColors(gui_colorset);
	gui_colorset.Unlock();
	
	dec->SetTitle(title);
	
	return dec;
}

DecorManager::DecorManager(void)
 : 	fDecorList(0),
 	fCurrentDecor(NULL)
{
	// Start with the default decorator - index is always 0
	DecorInfo *defaultDecor = new DecorInfo(-1, "Default", create_default_decorator);
	fDecorList.AddItem( defaultDecor );
	
	// Add any on disk
	RescanDecorators();
	
	// Find out which one should be the active one
	BDirectory dir;
	if (dir.SetTo(SERVER_SETTINGS_DIR) == B_ENTRY_NOT_FOUND)
		create_directory(SERVER_SETTINGS_DIR, 0777);
	
	BMessage settings;
	BFile file(SERVER_SETTINGS_DIR "decorator_settings", B_READ_ONLY);
	
	// Fallback to the default decorator if something goes wrong
	if (file.InitCheck() == B_OK && settings.Unflatten(&file) == B_OK)
	{
		BString itemtext;
		if(	settings.FindString("decorator", &itemtext) == B_OK ) 
		{
			fCurrentDecor = FindDecor(itemtext.String());
		}
	}
	
	if(!fCurrentDecor)
		fCurrentDecor = (DecorInfo*) fDecorList.ItemAt(0L);
}

DecorManager::~DecorManager(void)
{
	EmptyList();
}

void
DecorManager::RescanDecorators(void)
{
	BDirectory dir(DECORATORS_DIR);
	
	if(dir.InitCheck() != B_OK)
		return;
	
	entry_ref ref;
	BString fullpath;
	
	while(dir.GetNextRef(&ref)==B_OK)
	{
		fullpath = DECORATORS_DIR;
		fullpath += ref.name;
		
		// Because this function is used for both initialization and for keeping
		// the list up to date, check for existence in the list. Note that we
		// do not check to see if a decorator has been removed. This is for
		// stability. If there is a decorator in memory already whose file has
		// been deleted, it is still available until the next boot, at which point
		// it will obviously not be loaded.
		
		if( FindDecor(ref.name) )
			continue;
			
		image_id tempID = load_add_on(fullpath.String());
		if(tempID == B_ERROR)
			continue;
		
		// As of now, we do nothing with decorator versions, but the possibility
		// exists that the API will change even though I cannot forsee any reason
		// to do so. If we *did* do anything with decorator versions, the 
		// assignment would go here.
		
		create_decorator *createfunc;
		
		// Get the instantiation function
		status_t status = get_image_symbol(tempID, "instantiate_decorator",
								B_SYMBOL_TYPE_TEXT, (void**)&createfunc);
		
		if (status != B_OK) 
		{
			unload_add_on(tempID);
			continue;
		}
		
		fDecorList.AddItem( new DecorInfo(tempID, ref.name, createfunc) );
	}
}

Decorator *
DecorManager::AllocateDecorator(BRect rect, const char *title, 
								int32 wlook, int32 wfeel,
								int32 wflags, DisplayDriver *ddriver)
{
	// Create a new instance of the current decorator. Ownership is that of the caller
	
	if(!fCurrentDecor)
	{
		// We should *never* be here. If we do, it's a bug.
		debugger("DecorManager::AllocateDecorator has a NULL decorator");
		return NULL;
	}
	
	return fCurrentDecor->Instantiate(rect,title,wlook,wfeel,wflags,ddriver);
}

int32
DecorManager::CountDecorators(void) const
{
	return fDecorList.CountItems();
}

int32
DecorManager::GetDecorator(void) const
{
	return fDecorList.IndexOf( fCurrentDecor );
}

bool
DecorManager::SetDecorator(const int32 &index)
{
	DecorInfo *newDecInfo = (DecorInfo*) fDecorList.ItemAt(index);
	
	if(newDecInfo)
	{
		fCurrentDecor = newDecInfo;
		return true;
	}
	
	return false;
}

bool
DecorManager::SetR5Decorator(const int32 &value)
{
	BString string;
	
	switch(value)
	{
		case 0: string="BeOS"; break;
		case 1: string="AmigaOS"; break;
		case 2: string="Windows"; break;
		case 3: string="MacOS"; break;
		default:
			return false;
	}
	
	DecorInfo *newDecInfo = FindDecor(string.String());
	if(newDecInfo)
	{
		fCurrentDecor = newDecInfo;
		return true;
	}
	
	return false;
}

const char *
DecorManager::GetDecoratorName(const int32 &index)
{
	DecorInfo *info = (DecorInfo*) fDecorList.ItemAt(index);
	
	if(info)
		return info->GetName();
	
	return NULL;
}

void
DecorManager::EmptyList(void)
{
	for(int32 i=0; i < fDecorList.CountItems(); i++)
	{
		DecorInfo *info=(DecorInfo*)fDecorList.ItemAt(i);
		delete info;
	}
	
	fCurrentDecor = NULL;
}

DecorInfo*
DecorManager::FindDecor(const char *name)
{
	if(!name)
		return NULL;
	
	for(int32 i=0; i<fDecorList.CountItems(); i++)
	{
		DecorInfo *info = (DecorInfo*) fDecorList.ItemAt(i);
		
		if(info && strcmp(name, info->GetName())==0 )
			return info;
	}
	
	return NULL;
}

// This is the allocator function for the default decorator
Decorator* create_default_decorator(BRect rect, int32 wlook, int32 wfeel, int32 wflags)
{
	return new DefaultDecorator(rect,wlook,wfeel,wflags);
}

