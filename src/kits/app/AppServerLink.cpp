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
//	File Name:		AppServerLink.cpp
//	Author(s):		Erik Jaesler (erik@cgsoftware.com)
//	Description:	BAppServerLink provides proxied access to the application's
//					connection with the app_server.  It has BAutolock semantics:
//					creating one locks the app_server connection; destroying one
//					unlocks the connection.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Application.h>

// Project Includes ------------------------------------------------------------
#include <AppServerLink.h>

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

namespace BPrivate {

BAppServerLink::BAppServerLink(void)
 : BPortLink()
{
	if (be_app)
	{
		be_app->Lock();
		SetSendPort(be_app->fServerTo);
	}
	receiver=create_port(100,"AppServerLink reply port");
	SetReplyPort(receiver);

}

//------------------------------------------------------------------------------

BAppServerLink::~BAppServerLink()
{
	delete_port(receiver);
	if (be_app)
		be_app->Unlock();
}

//------------------------------------------------------------------------------

status_t BAppServerLink::FlushWithReply(int32 *code)
{
	status_t err;
	
	err = Attach<port_id>(receiver);
	if (err < B_OK)
		return err;

	err = Flush();
	if (err < B_OK)
		return err;

	return GetNextReply(code);
}

}	// namespace BPrivate

/*
 * $Log $
 *
 * $Id  $
 *
 */

