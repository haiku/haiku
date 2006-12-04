/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author: DarkWyrm <bpmagic@columbus.rr.com>
 */
 

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Message.h>
#include <Path.h>
#include <Rect.h>

#include "AppServer.h"
#include "DefaultDecorator.h"
#include "Desktop.h"
#include "DesktopSettings.h"
#include "ServerConfig.h"
#include "DecorManager.h"


// Globals
DecorManager gDecorManager;


// This is a class used only by the DecorManager to track all the decorators in memory
class DecorInfo {
	public:
							DecorInfo(image_id id, const char* name,
								create_decorator* allocator = NULL);
							~DecorInfo();

		status_t			InitCheck() const;

		image_id			ID() const { return fID; }
		const char*			Name() const { return fName.String(); }

		Decorator*			Instantiate(Desktop* desktop, BRect rect, const char* title,
								window_look look, uint32 flags);

	private:
		image_id			fID;
		BString 			fName;
		create_decorator	*fAllocator;
};


DecorInfo::DecorInfo(image_id id, const char* name, create_decorator* allocator)
	:
	fID(id),
 	fName(name),
 	fAllocator(allocator)
{
}


DecorInfo::~DecorInfo()
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
DecorInfo::Instantiate(Desktop* desktop, BRect rect, const char *title,
	window_look look, uint32 flags)
{
	if (!desktop->LockSingleWindow())
		return NULL;

	DesktopSettings settings(desktop);
	Decorator *decorator;
	
	try {
		if (fAllocator != NULL)
			decorator = fAllocator(settings, rect, look, flags);
		else
			decorator = new DefaultDecorator(settings, rect, look, flags);
	} catch (...) {
		desktop->UnlockSingleWindow();
		return NULL;
	}

	desktop->UnlockSingleWindow();

	decorator->SetDrawingEngine(desktop->GetDrawingEngine());
	decorator->SetTitle(title);

	return decorator;
}


//	#pragma mark -


DecorManager::DecorManager()
 :	BLocker("DecorManager"),
	fDecorList(0),
 	fCurrentDecor(NULL)
{
	// Start with the default decorator - index is always 0
	DecorInfo *defaultDecor = new DecorInfo(-1, "Default", NULL);
	fDecorList.AddItem(defaultDecor);

#if 0
	// Add any on disk
	RescanDecorators();

	// Find out which one should be the active one
	BDirectory dir;
	if (dir.SetTo(SERVER_SETTINGS_DIR) == B_ENTRY_NOT_FOUND)
		create_directory(SERVER_SETTINGS_DIR, 0777);

	BMessage settings;
	BFile file(SERVER_SETTINGS_DIR "decorator_settings", B_READ_ONLY);

	// Fallback to the default decorator if something goes wrong
	if (file.InitCheck() == B_OK && settings.Unflatten(&file) == B_OK) {
		BString itemtext;
		if (settings.FindString("decorator", &itemtext) == B_OK) {
			fCurrentDecor = _FindDecor(itemtext.String());
		}
	}
#endif
	if (!fCurrentDecor)
		fCurrentDecor = (DecorInfo*)fDecorList.ItemAt(0L);
}


DecorManager::~DecorManager()
{
	_EmptyList();
}


void
DecorManager::RescanDecorators()
{
	BDirectory dir(DECORATORS_DIR);

	if (dir.InitCheck() != B_OK)
		return;

	entry_ref ref;
	while (dir.GetNextRef(&ref) == B_OK) {
		BPath path;
		path.SetTo(DECORATORS_DIR);
		path.Append(ref.name);

		// Because this function is used for both initialization and for keeping
		// the list up to date, check for existence in the list. Note that we
		// do not check to see if a decorator has been removed. This is for
		// stability. If there is a decorator in memory already whose file has
		// been deleted, it is still available until the next boot, at which point
		// it will obviously not be loaded.

		if (_FindDecor(ref.name))
			continue;

		image_id image = load_add_on(path.Path());
		if (image != B_OK)
			continue;

		// As of now, we do nothing with decorator versions, but the possibility
		// exists that the API will change even though I cannot forsee any reason
		// to do so. If we *did* do anything with decorator versions, the 
		// assignment would go here.

		create_decorator* createFunc;

		// Get the instantiation function
		status_t status = get_image_symbol(image, "instantiate_decorator",
								B_SYMBOL_TYPE_TEXT, (void**)&createFunc);
		if (status != B_OK) {
			unload_add_on(image);
			continue;
		}

		// TODO: unload images until they are actually used!
		fDecorList.AddItem(new DecorInfo(image, ref.name, createFunc));
	}
}


Decorator *
DecorManager::AllocateDecorator(Desktop* desktop, BRect rect, const char *title, 
	window_look look, uint32 flags)
{
	// Create a new instance of the current decorator. Ownership is that of the caller

	if (!fCurrentDecor) {
		// We should *never* be here. If we do, it's a bug.
		debugger("DecorManager::AllocateDecorator has a NULL decorator");
		return NULL;
	}

	return fCurrentDecor->Instantiate(desktop, rect, title, look, flags);
}


int32
DecorManager::CountDecorators() const
{
	return fDecorList.CountItems();
}


int32
DecorManager::GetDecorator() const
{
	return fDecorList.IndexOf(fCurrentDecor);
}


bool
DecorManager::SetDecorator(int32 index)
{
	DecorInfo *newDecInfo = (DecorInfo*)fDecorList.ItemAt(index);

	if (newDecInfo) {
		fCurrentDecor = newDecInfo;
		return true;
	}

	return false;
}


bool
DecorManager::SetR5Decorator(int32 value)
{
	BString string;

	switch (value) {
		case 0: string = "BeOS"; break;
		case 1: string = "AmigaOS"; break;
		case 2: string = "Windows"; break;
		case 3: string = "MacOS"; break;
		default:
			return false;
	}

	DecorInfo *newDecInfo = _FindDecor(string.String());
	if (newDecInfo) {
		fCurrentDecor = newDecInfo;
		return true;
	}

	return false;
}


const char *
DecorManager::GetDecoratorName(int32 index)
{
	DecorInfo *info = fDecorList.ItemAt(index);
	if (info)
		return info->Name();

	return NULL;
}


void
DecorManager::_EmptyList()
{
	for (int32 i = 0; i < fDecorList.CountItems(); i++) {
		delete fDecorList.ItemAt(i);;
	}

	fDecorList.MakeEmpty();
	fCurrentDecor = NULL;
}


DecorInfo*
DecorManager::_FindDecor(const char *name)
{
	if (!name)
		return NULL;

	for (int32 i = 0; i < fDecorList.CountItems(); i++) {
		DecorInfo* info = fDecorList.ItemAt(i);

		if (strcmp(name, info->Name()) == 0)
			return info;
	}

	return NULL;
}

