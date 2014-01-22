/*
 * Copyright 2001-2013, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 *		Jérôme Duval, jerome.duval@free.fr
 *		Andrej Spielmann, <andrej.spielmann@seh.ox.ac.uk>
 *		Philippe Saint-Pierre, stpere@gmail.com
 *		Wim van der Meer, <WPJvanderMeer@gmail.com>
 */


#include "ServerApp.h"

#include <syslog.h>

#include <ServerProtocol.h>


/* These methods are split to a separate compilation unit to allow splitting
 * the test_app_server and libtestappserver.so on a convenient boundary.
 */


/*!	Called from the ClientMemoryAllocator whenever a server area could be
	deleted.
	A message is then sent to the client telling it that it can delete its
	client area, too.
*/
void
ServerApp::NotifyDeleteClientArea(area_id serverArea)
{
	BMessage notify(kMsgDeleteServerMemoryArea);
	notify.AddInt32("server area", serverArea);

	SendMessageToClient(&notify);
}


/*!	\brief Send a message to the ServerApp's BApplication
	\param message The message to send
*/
void
ServerApp::SendMessageToClient(BMessage* message) const
{
	status_t status = fHandlerMessenger.SendMessage(message, (BHandler*)NULL,
		100000);
	if (status != B_OK) {
		syslog(LOG_ERR, "app %s send to client failed: %s\n", Signature(),
			strerror(status));
	}
}
