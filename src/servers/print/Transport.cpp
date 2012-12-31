/*
 * Copyright 2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema
 */

#include "Transport.h"

// BeOS API
#include <PrintTransportAddOn.h>
#include <Application.h>
#include <image.h>
#include <Entry.h>


BObjectList<Transport> Transport::sTransports;


// ---------------------------------------------------------------
// Find [static]
//
// Searches the static object list for a transport object with the
// specified name.
//
// Parameters:
//    name - Printer definition name we're looking for.
//
// Returns:
//    Pointer to Transport object, or NULL if not found.
// ---------------------------------------------------------------
Transport* 
Transport::Find(const BString& name)
{
		// Look in list to find printer definition
	for (int32 index = 0; index < sTransports.CountItems(); index++) {
		if (name == sTransports.ItemAt(index)->Name())
			return sTransports.ItemAt(index);
	}
	
		// None found, so return NULL
	return NULL;
}


Transport* 
Transport::At(int32 index)
{
	return sTransports.ItemAt(index);
}


void 
Transport::Remove(Transport* transport)
{
	sTransports.RemoveItem(transport);
}


int32 
Transport::CountTransports()
{
	return sTransports.CountItems();
}


status_t 
Transport::Scan(directory_which which)
{
	status_t result;
	BPath path;

	// Try to find specified transport addon directory
	if ((result = find_directory(which, &path)) != B_OK)
		return result;

	if ((result = path.Append("Print/transport")) != B_OK)
		return result;

	BDirectory dir;
	if ((result = dir.SetTo(path.Path())) != B_OK)
		return result;

	// Walk over all entries in directory
	BEntry entry;
	while(dir.GetNextEntry(&entry) == B_OK) {
		if (!entry.IsFile())
			continue;

		if (entry.GetPath(&path) != B_OK)
			continue;

		// If we have loaded the transport from a previous scanned directory,
		// ignore this one.
		if (Transport::Find(path.Leaf()) != NULL)
			continue;

		be_app->AddHandler(new Transport(path));
	}

	return B_OK;
}


// ---------------------------------------------------------------
// Transport [constructor]
//
// Initializes the transport object with data read from the
// attributes attached to the printer definition node.
//
// Parameters:
//    node - Printer definition node for this printer.
//
// Returns:
//    none.
// ---------------------------------------------------------------
Transport::Transport(const BPath& path)
	: BHandler(B_EMPTY_STRING),
	fPath(path),
	fImageID(-1),
	fFeatures(0)
{
	// Load transport addon
	image_id id = ::load_add_on(path.Path());
	if (id < B_OK)
		return;

	// Find transport_features symbol, to determine if we need to keep 
	// this transport loaded
	int* transportFeaturesPointer;
	if (get_image_symbol(id, B_TRANSPORT_FEATURES_SYMBOL, 
			B_SYMBOL_TYPE_DATA, (void**)&transportFeaturesPointer) != B_OK) {
		unload_add_on(id);
	} else {
		fFeatures = *transportFeaturesPointer;

		if (fFeatures & B_TRANSPORT_IS_HOTPLUG) {
			// We are hotpluggable; so keep us loaded!
			fImageID = id;
		} else {
			// No extended Transport support; so no need to keep loaded
			::unload_add_on(id);
		}
	}

	sTransports.AddItem(this);
}


Transport::~Transport()
{
	sTransports.RemoveItem(this);
}


status_t
Transport::ListAvailablePorts(BMessage* msg)
{
	status_t (*list_ports)(BMessage*);
	image_id id = fImageID;
	status_t rc = B_OK;

	// Load image if not loaded yet
	if (id == -1 && (id = load_add_on(fPath.Path())) < 0)
		return id;

	// Get pointer to addon function
	if ((rc = get_image_symbol(id, B_TRANSPORT_LIST_PORTS_SYMBOL,
			B_SYMBOL_TYPE_TEXT, (void**)&list_ports)) != B_OK)
		goto done;

	// run addon...
	rc = (*list_ports)(msg);

done:
	// clean up if needed
	if (fImageID != id)
		unload_add_on(id);

	return rc;
}


// ---------------------------------------------------------------
// MessageReceived
//
// Handle scripting messages.
//
// Parameters:
//    msg - message.
// ---------------------------------------------------------------
void 
Transport::MessageReceived(BMessage* msg)
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
