/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */


#include "WindowStack.h"

#include <new>

#include <Window.h>

#include <ApplicationPrivate.h>
#include <MessengerPrivate.h>
#include <PortLink.h>
#include <ServerProtocol.h>

#include "StackAndTilePrivate.h"


using namespace BPrivate;


BWindowStack::BWindowStack(BWindow* window)
	:
	fLink(NULL)
{
	port_id receivePort = create_port(B_LOOPER_PORT_DEFAULT_CAPACITY,
		"w_stack<app_server");
	if (receivePort != B_OK)
		fLink = new(std::nothrow) BPrivate::PortLink(
			window->fLink->SenderPort(), receivePort);
}


BWindowStack::~BWindowStack()
{
	if (fLink)
		delete_port(fLink->ReceiverPort());
	delete fLink;
}


status_t
BWindowStack::InitCheck()
{
	if (!fLink)
		return B_NO_MEMORY;
	return B_OK;
}


status_t
BWindowStack::AddWindow(BWindow* window)
{
	BMessenger messenger(window);
	return AddWindow(messenger);
}


status_t
BWindowStack::AddWindow(BMessenger& window)
{
	return AddWindowAt(window, -1);
}


status_t
BWindowStack::AddWindowAt(BWindow* window, int32 position)
{
	BMessenger messenger(window);
	return AddWindowAt(messenger, position);
}


status_t
BWindowStack::AddWindowAt(BMessenger& window, int32 position)
{
	_StartMessage(kAddWindowToStack);

	_AttachMessenger(window);
	fLink->Attach<int32>(position);

	int32 code = B_ERROR;
	if (fLink->FlushWithReply(code) != B_OK)
		return code;

	return B_OK;
}


status_t
BWindowStack::RemoveWindow(BWindow* window)
{
	BMessenger messenger(window);
	return RemoveWindow(messenger);
}


status_t
BWindowStack::RemoveWindow(BMessenger& window)
{
	_StartMessage(kRemoveWindowFromStack);
	_AttachMessenger(window);

	if (fLink->Flush() != B_OK)
		return B_ERROR;

	return B_OK;
}


status_t
BWindowStack::RemoveWindowAt(int32 position, BMessenger* window)
{
	_StartMessage(kRemoveWindowFromStackAt);
	fLink->Attach<int32>(position);

	int32 code = B_ERROR;
	if (fLink->FlushWithReply(code) != B_OK)
		return code;

	if (window == NULL)
		return B_OK;

	return _ReadMessenger(*window);
}


int32
BWindowStack::CountWindows()
{
	_StartMessage(kCountWindowsOnStack);

	int32 code = B_ERROR;
	fLink->FlushWithReply(code);
	if (code != B_OK)
		return -1;

	int32 count;
	if (fLink->Read<int32>(&count) != B_OK)
		return -1;

	return count;
}


status_t
BWindowStack::WindowAt(int32 position, BMessenger& messenger)
{
	_StartMessage(kWindowOnStackAt);
	fLink->Attach<int32>(position);

	int32 code = B_ERROR;
	fLink->FlushWithReply(code);
	if (code != B_OK)
		return code;

	return _ReadMessenger(messenger);
}


bool
BWindowStack::HasWindow(BWindow* window)
{
	BMessenger messenger(window);
	return HasWindow(messenger);
}


bool
BWindowStack::HasWindow(BMessenger& window)
{
	_StartMessage(kStackHasWindow);
	_AttachMessenger(window);

	int32 code = B_ERROR;
	fLink->FlushWithReply(code);
	if (code != B_OK)
		return code;

	bool hasWindow;
	if (fLink->Read<bool>(&hasWindow) != B_OK)
		return false;
		
	return hasWindow;
}


status_t
BWindowStack::_AttachMessenger(BMessenger& window)
{
	BMessenger::Private messengerPrivate(window);
	fLink->Attach<port_id>(messengerPrivate.Port());
	fLink->Attach<int32>(messengerPrivate.Token());
	return fLink->Attach<team_id>(messengerPrivate.Team());
}


status_t
BWindowStack::_ReadMessenger(BMessenger& window)
{
	port_id port;
	int32 token;
	team_id team;
	fLink->Read<port_id>(&port);
	fLink->Read<int32>(&token);
	status_t status = fLink->Read<team_id>(&team);
	if (status != B_OK)
		return status;
	BMessenger::Private messengerPrivate(window);
	messengerPrivate.SetTo(team, port, token);
	return B_OK;
}


status_t
BWindowStack::_StartMessage(int32 what)
{
	fLink->StartMessage(AS_TALK_TO_DESKTOP_LISTENER);
	fLink->Attach<port_id>(fLink->ReceiverPort());
	fLink->Attach<int32>(kMagicSATIdentifier);
	return fLink->Attach<int32>(what);
}
