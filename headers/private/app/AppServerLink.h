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
//	File Name:		AppServerLink.h
//	Author(s):		Erik Jaesler (erik@cgsoftware.com)
//	Description:	BAppServerLink provides proxied access to the application's
//					connection with the app_server.  It has BAutolock semantics:
//					creating one locks the app_server connection; destroying one
//					unlocks the connection.
//------------------------------------------------------------------------------

#ifndef APPSERVERLINK_H
#define APPSERVERLINK_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------
#include <Session.h>
#include <OS.h>

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//class PortLink;

namespace BPrivate {

class BAppServerLink : public BSession
{
public:
	BAppServerLink(void);
	~BAppServerLink(void);
private:
	port_id receiver;
};

}	// namespace BPrivate

#endif	//APPSERVERLINK_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

