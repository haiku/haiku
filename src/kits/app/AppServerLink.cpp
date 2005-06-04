//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku
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

#include <Application.h>
#include <Locker.h>

#include <AppServerLink.h>


BLocker sLock;
port_id sReplyPort = -1;


namespace BPrivate {

BAppServerLink::BAppServerLink(void)
{
	sLock.Lock();

	// if there is no be_app, we can't do a whole lot, anyway
	if (be_app)
		SetSendPort(be_app->fServerFrom);

	// There is only one global reply port, and we create it here
	// (protected by sLock) when it's not yet there
	if (sReplyPort < B_OK)
		sReplyPort = create_port(100, "AppServerLink reply port");

	SetReplyPort(sReplyPort);
}


BAppServerLink::~BAppServerLink()
{
	sLock.Unlock();
}


status_t
BAppServerLink::FlushWithReply(int32 *code)
{
	status_t err;
	
	err = Attach<port_id>(sReplyPort);
	if (err < B_OK)
		return err;

	err = Flush();
	if (err < B_OK)
		return err;

	return GetNextReply(code);
}

}	// namespace BPrivate
