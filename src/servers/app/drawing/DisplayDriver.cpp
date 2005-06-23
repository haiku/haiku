//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc. All rights reserved
//  Distributed under the terms of the MIT license.
//
//	File Name:		DisplayDriver.cpp
//	Authors:		DarkWyrm <bpmagic@columbus.rr.com>
//					Gabe Yoder <gyoder@stny.rr.com>
//					Stephan Aßmus <superstippi@gmx.de>
//
//	Description:	Abstract class which handles all graphics output
//					for the server
//  
//------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>


#include "DisplayDriver.h"

DisplayDriver::DisplayDriver()
{
}

DisplayDriver::~DisplayDriver()
{
}

/*!
	\brief Initializes the driver object.
	\return true if successful, false if not
	
	Derived versions should set up the driver for display,
	including the initial clearing of the screen. If things
	do not go as they should, false should be returned.
*/
status_t
DisplayDriver::Initialize()
{
	return B_OK;
}

/*!
	\brief Shuts down the driver's video subsystem
	
	Any work done by Initialize() should be undone here. Note that Shutdown() is
	called even if Initialize() was unsuccessful.
*/
void
DisplayDriver::Shutdown()
{
}

