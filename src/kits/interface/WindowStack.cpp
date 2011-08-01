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
{
	fLink = window->fLink;
}


BWindowStack::~BWindowStack()
{

}


status_t
BWindowStack::AddWindow(const BWindow* window)
{
	BMessenger messenger(window);
	return AddWindow(messenger);
}


status_t
BWindowStack::AddWindow(const BMessenger& window)
{
	return AddWindowAt(window, -1);
}


status_t
BWindowStack::AddWindowAt(const BWindow* window, int32 position)
{
	BMessenger messenger(window);
	return AddWindowAt(messenger, position);
}


status_t
BWindowStack::AddWindowAt(const BMessenger& window, int32 position)
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
BWindowStack::RemoveWindow(const BWindow* window)
{
	BMessenger messenger(window);
	return RemoveWindow(messenger);
}


status_t
BWindowStack::RemoveWindow(const BMessenger& window)
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
BWindowStack::HasWindow(const BWindow* window)
{
	BMessenger messenger(window);
	return HasWindow(messenger);
}


bool
BWindowStack::HasWindow(const BMessenger& window)
{
	_StartMessage(kStackHasWindow);
	_AttachMessenger(window);

	int32 code = B_ERROR;
	fLink->FlushWithReply(code);
	if (code != B_OK)
		return false;

	bool hasWindow;
	if (fLink->Read<bool>(&hasWindow) != B_OK)
		return false;
		
	return hasWindow;
}


status_t
BWindowStack::_AttachMessenger(const BMessenger& window)
{
	BMessenger::Private messengerPrivate(const_cast<BMessenger&>(window));
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
	fLink->Attach<int32>(kMagicSATIdentifier);
	fLink->Attach<int32>(kStacking);
	return fLink->Attach<int32>(what);
}
