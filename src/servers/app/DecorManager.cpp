/*
 * Copyright (c) 2001-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */

#include "DecorManager.h"

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>
#include <Rect.h>

#include <syslog.h>

#include "AppServer.h"
#include "DefaultDecorator.h"
#include "DefaultWindowBehaviour.h"
#include "Desktop.h"
#include "DesktopSettings.h"
#include "ServerConfig.h"
#include "Window.h"

typedef float get_version(void);
typedef DecorAddOn* create_decor_addon(image_id id, const char* name);

// Globals
DecorManager gDecorManager;


DecorAddOn::DecorAddOn(image_id id, const char* name)
	:
	fImageID(id),
 	fName(name)
{
}


DecorAddOn::~DecorAddOn()
{

}


status_t
DecorAddOn::InitCheck() const
{
	return B_OK;
}


Decorator*
DecorAddOn::AllocateDecorator(Desktop* desktop, DrawingEngine* engine,
	BRect rect, const char* title, window_look look, uint32 flags)
{
	if (!desktop->LockSingleWindow())
		return NULL;

	DesktopSettings settings(desktop);
	Decorator* decorator;
	decorator = _AllocateDecorator(settings, rect, look, flags);

	desktop->UnlockSingleWindow();

	if (!decorator)
		return NULL;
	decorator->SetDrawingEngine(engine);
	decorator->SetTitle(title);

	return decorator;
}


WindowBehaviour*
DecorAddOn::AllocateWindowBehaviour(Window* window)
{
	return new (std::nothrow)DefaultWindowBehaviour(window);
}


const DesktopListenerList&
DecorAddOn::GetDesktopListeners()
{
	return fDesktopListeners;
}


Decorator*
DecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	window_look look, uint32 flags)
{
	return new (std::nothrow)DefaultDecorator(settings, rect, look, flags);
}


//	#pragma mark -


DecorManager::DecorManager()
	:
	fDefaultDecorAddOn(-1, "Default"),
	fCurrentDecor(NULL)
{
	// Start with the default decorator - index is always 0
	fDecorList.AddItem(&fDefaultDecorAddOn);

	// Add any on disk
	RescanDecorators();

	_LoadSettingsFromDisk();

	if (!fCurrentDecor)
		fCurrentDecor = fDecorList.ItemAt(0L);
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
		if (image < 0)
			continue;

		// As of now, we do nothing with decorator versions, but the possibility
		// exists that the API will change even though I cannot forsee any reason
		// to do so. If we *did* do anything with decorator versions, the 
		// assignment would go here.

		create_decor_addon* createFunc;

		// Get the instantiation function
		status_t status = get_image_symbol(image, "instantiate_decor_addon",
								B_SYMBOL_TYPE_TEXT, (void**)&createFunc);
		if (status != B_OK) {
			unload_add_on(image);
			continue;
		}

		DecorAddOn* addon = createFunc(image, ref.name);

		// TODO: unload images until they are actually used!
		if (!addon || addon->InitCheck() != B_OK
			|| !fDecorList.AddItem(addon)) {
			unload_add_on(image);
			delete addon;
			continue;
		}
	}
}


Decorator *
DecorManager::AllocateDecorator(Desktop* desktop, DrawingEngine* engine,
	BRect rect, const char* title, window_look look, uint32 flags)
{
	// Create a new instance of the current decorator.
	// Ownership is that of the caller

	if (!fCurrentDecor) {
		// We should *never* be here. If we do, it's a bug.
		debugger("DecorManager::AllocateDecorator has a NULL decorator");
		return NULL;
	}

	return fCurrentDecor->AllocateDecorator(desktop, engine, rect, title,
		look, flags);
}


WindowBehaviour*
DecorManager::AllocateWindowBehaviour(Window* window)
{
	if (!fCurrentDecor) {
		// We should *never* be here. If we do, it's a bug.
		debugger("DecorManager::AllocateDecorator has a NULL decorator");
		return NULL;
	}

	return fCurrentDecor->AllocateWindowBehaviour(window);
}


const DesktopListenerList&
DecorManager::GetDesktopListeners()
{
	return fCurrentDecor->GetDesktopListeners();
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
DecorManager::SetDecorator(int32 index, Desktop* desktop)
{
	DecorAddOn* newDecor = fDecorList.ItemAt(index);

	if (newDecor) {
		fCurrentDecor = newDecor;
		desktop->ReloadDecor();
		_SaveSettingsToDisk();
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

	DecorAddOn *newDecor = _FindDecor(string);
	if (newDecor) {
		fCurrentDecor = newDecor;
		return true;
	}

	return false;
}


BString
DecorManager::GetDecoratorName(int32 index)
{
	DecorAddOn *decor = fDecorList.ItemAt(index);
	if (decor)
		return decor->Name();

	return BString("");
}


void
DecorManager::_EmptyList()
{
	for (int32 i = 1; i < fDecorList.CountItems(); i++) {
		unload_add_on(fDecorList.ItemAt(i)->ImageID());
		delete fDecorList.ItemAt(i);
	}

	fDecorList.MakeEmpty();
	fDecorList.AddItem(&fDefaultDecorAddOn);

	fCurrentDecor = &fDefaultDecorAddOn;
}


DecorAddOn*
DecorManager::_FindDecor(BString name)
{
	if (!name)
		return NULL;

	for (int32 i = 0; i < fDecorList.CountItems(); i++) {
		DecorAddOn* decor = fDecorList.ItemAt(i);

		if (decor->Name() == name)
			return decor;
	}

	return NULL;
}


static const char* kSettingsDir = "system/app_server";
static const char* kSettingsFile = "decorator_settings";


bool
DecorManager::_LoadSettingsFromDisk()
{
	// get the user settings directory
	BPath path;
	status_t error = find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	if (error != B_OK)
		return false;

	path.Append(kSettingsDir);
	path.Append(kSettingsFile);
	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return false;

	BMessage settings;
	if (settings.Unflatten(&file) == B_OK) {
		BString itemtext;
		if (settings.FindString("decorator", &itemtext) == B_OK) {
			DecorAddOn* decor = _FindDecor(itemtext);
			if (decor) {
				fCurrentDecor = decor;
				return true;
			}
		}
	}

	return false;
}


bool
DecorManager::_SaveSettingsToDisk()
{
	// get the user settings directory
	BPath path;
	status_t error = find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	if (error != B_OK)
		return false;

	path.Append(kSettingsDir);
	if (create_directory(path.Path(), 777) != B_OK)
		return false;

	path.Append(kSettingsFile);
	BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() != B_OK)
		return false;

	BMessage settings;
	if (settings.AddString("decorator", fCurrentDecor->Name()) != B_OK)
		return false;
	if (settings.Flatten(&file) != B_OK)
		return false;

	return true;
}

