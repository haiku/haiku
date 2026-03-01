//------------------------------------------------------------------------------
//	RemoteTestObject.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <iostream>

// System Includes -------------------------------------------------------------
#include <Message.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "RemoteTestObject.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
TRemoteTestObject::TRemoteTestObject(int32 i)
	:	data(i)
{
	;
}
//------------------------------------------------------------------------------
TRemoteTestObject::TRemoteTestObject(BMessage *archive)
{
	data = archive->FindInt32("TRemoteTestObject::data");
}
//------------------------------------------------------------------------------
status_t TRemoteTestObject::Archive(BMessage *archive, bool deep)
{
	status_t err = archive->AddString("class", "TRemoteTestObject");

	if (!err)
		err = archive->AddInt32("TRemoteTestObject::data", data);

	return err;
}
//------------------------------------------------------------------------------
TRemoteTestObject* TRemoteTestObject::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "TRemoteTestObject"))
		return new TRemoteTestObject(archive);
	return NULL;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

