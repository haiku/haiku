/*****************************************************************************/
// print_server Background Application.
//
// Version: 1.0.0d1
//
// The print_server manages the communication between applications and the
// printer and transport drivers.
//
//
//
// TODO:
//	 Handle spooled jobs at startup
//	 Handle printer status (asked to print a spooled job but printer busy)
//   
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include "Printer.h"

#include "pr_server.h"
#include "BeUtils.h"

	// BeOS API
#include <Application.h>
#include <Message.h>
#include <String.h>
#include <File.h>
#include <Path.h>

// ---------------------------------------------------------------
typedef BMessage* (*config_func_t)(BNode*, const BMessage*);
typedef BMessage* (*take_job_func_t)(BFile*, BNode*, const BMessage*);
typedef char* (*add_printer_func_t)(const char* printer_name);

// ---------------------------------------------------------------
BObjectList<Printer> Printer::sPrinters;


// ---------------------------------------------------------------
// Find [static]
//
// Searches the static object list for a printer object with the
// specified name.
//
// Parameters:
//    name - Printer definition name we're looking for.
//
// Returns:
//    Pointer to Printer object, or NULL if not found.
// ---------------------------------------------------------------
Printer* Printer::Find(const BString& name)
{
		// Look in list to find printer definition
	for (int32 idx=0; idx < sPrinters.CountItems(); idx++) {
		if (name == sPrinters.ItemAt(idx)->Name()) {
			return sPrinters.ItemAt(idx);
		}
	}
	
		// None found, so return NULL
	return NULL;
}

int32 Printer::CountPrinters()
{
	return sPrinters.CountItems();
}

Printer* Printer::At(int32 idx)
{
	return sPrinters.ItemAt(idx);
}

// ---------------------------------------------------------------
// Printer [constructor]
//
// Initializes the printer object with data read from the
// attributes attached to the printer definition node.
//
// Parameters:
//    node - Printer definition node for this printer.
//
// Returns:
//    none.
// ---------------------------------------------------------------
Printer::Printer(const BNode* node)
	: Inherited(B_EMPTY_STRING),
	fNode(*node)
{
		// Set our name to the name of the passed node
	BString name;
	fNode.ReadAttrString(PSRV_PRINTER_ATTR_PRT_NAME, &name);
	SetName(name.String());
	
		// Add us to the global list of known printer definitions
	sPrinters.AddItem(this);
	be_app->AddHandler(this);
}

Printer::~Printer()
{
	sPrinters.RemoveItem(this);
	be_app->RemoveHandler(this);
}

status_t Printer::Remove()
{
	status_t rc = B_OK;
	BPath path;

	if ((rc=::find_directory(B_USER_PRINTERS_DIRECTORY, &path)) == B_OK) {
		path.Append(Name());
		rc = rmdir(path.Path());
	}
	
	return rc;
}

// ---------------------------------------------------------------
// ConfigurePrinter
//
// Handles calling the printer addon's add_printer function.
//
// Parameters:
//    none.
//
// Returns:
//    B_OK if successful or errorcode otherwise.
// ---------------------------------------------------------------
status_t Printer::ConfigurePrinter()
{
	image_id id;
	status_t rc;
	
	if ((rc=LoadPrinterAddon(id)) == B_OK) {
			// Addon was loaded, so try and get the add_printer symbol
		add_printer_func_t func;

		if (get_image_symbol(id, "add_printer", B_SYMBOL_TYPE_TEXT, (void**)&func) == B_OK) {
				// call the function and check its result
			rc = ((*func)(Name()) == NULL) ? B_ERROR : B_OK;
		}
		
		::unload_add_on(id);
	}
	
	return rc;
}

// ---------------------------------------------------------------
// ConfigurePage
//
// Handles calling the printer addon's config_page function.
//
// Parameters:
//    settings - Page settings to display. The contents of this
//               message will be replaced with the new settings
//               if the function returns success.
//
// Returns:
//    B_OK if successful or errorcode otherwise.
// ---------------------------------------------------------------
status_t Printer::ConfigurePage(BMessage& settings)
{
	image_id id;
	status_t rc;
	
	if ((rc=LoadPrinterAddon(id)) == B_OK) {
			// Addon was loaded, so try and get the config_page symbol
		config_func_t func;

		if ((rc=get_image_symbol(id, "config_page", B_SYMBOL_TYPE_TEXT, (void**)&func)) == B_OK) {
				// call the function and check its result
			BMessage* new_settings = (*func)(&fNode, &settings);
			if (new_settings != NULL && new_settings->what != 'baad')
				settings = *new_settings;
		}
		
		::unload_add_on(id);
	}
	
	return rc;
}

// ---------------------------------------------------------------
// ConfigureJob
//
// Handles calling the printer addon's config_job function.
//
// Parameters:
//    settings - Job settings to display. The contents of this
//               message will be replaced with the new settings
//               if the function returns success.
//
// Returns:
//    B_OK if successful or errorcode otherwise.
// ---------------------------------------------------------------
status_t Printer::ConfigureJob(BMessage& settings)
{
	image_id id;
	status_t rc;
	
	if ((rc=LoadPrinterAddon(id)) == B_OK) {
			// Addon was loaded, so try and get the config_job symbol
		config_func_t func;

		if ((rc=get_image_symbol(id, "config_job", B_SYMBOL_TYPE_TEXT, (void**)&func)) == B_OK) {
				// call the function and check its result
			BMessage* new_settings = (*func)(&fNode, &settings);
			if (new_settings != NULL && new_settings->what != 'baad')
				settings = *new_settings;
		}
		
		::unload_add_on(id);
	}
	
	return rc;
}

status_t Printer::PrintSpooledJob(BFile* spoolFile, const BMessage& settings)
{
	take_job_func_t func;
	image_id id;
	status_t rc;
	
	if ((rc=LoadPrinterAddon(id)) == B_OK) {
			// Addon was loaded, so try and get the take_job symbol
		if ((rc=get_image_symbol(id, "take_job", B_SYMBOL_TYPE_TEXT, (void**)&func)) == B_OK) {
				// call the function and check its result
			BMessage* result = (*func)(spoolFile, &fNode, &settings);
			if (result == NULL || result->what == 'baad')
				rc = B_ERROR;
		}
		
		::unload_add_on(id);
	}
	
	return rc;
}

// ---------------------------------------------------------------
// LoadPrinterAddon
//
// Try to load the printer addon into memory.
//
// Parameters:
//    id - image_id set to the image id of the loaded addon.
//
// Returns:
//    B_OK if successful or errorcode otherwise.
// ---------------------------------------------------------------
status_t Printer::LoadPrinterAddon(image_id& id)
{
	BString drName;
	status_t rc;
	BPath path;

	if ((rc=fNode.ReadAttrString(PSRV_PRINTER_ATTR_DRV_NAME, &drName)) == B_OK) {
			// try to locate the driver
		if ((rc=::TestForAddonExistence(drName.String(), B_USER_ADDONS_DIRECTORY, "Print", path)) != B_OK) {
			if ((rc=::TestForAddonExistence(drName.String(), B_COMMON_ADDONS_DIRECTORY, "Print", path)) != B_OK) {
				rc = ::TestForAddonExistence(drName.String(), B_BEOS_ADDONS_DIRECTORY, "Print", path);
			}
		}
		
			// If the driver was found
		if (rc == B_OK) {
				// If we cannot load the addon
			if ((id=::load_add_on(path.Path())) < 0)
				rc = id;
		}
	}	
	return rc;
}

void Printer::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case B_GET_PROPERTY:
		case B_SET_PROPERTY:
		case B_CREATE_PROPERTY:
		case B_DELETE_PROPERTY:
		case B_COUNT_PROPERTIES:
		case B_EXECUTE_PROPERTY:
			HandleScriptingCommand(msg);
			break;
		
		default:
			Inherited::MessageReceived(msg);
	}
}
