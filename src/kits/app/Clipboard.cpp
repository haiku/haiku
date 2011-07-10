/*
 * Copyright 2001-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Gabe Yoder (gyoder@stny.rr.com)
 */


#include <Clipboard.h>

#include <Application.h>
#include <RegistrarDefs.h>
#include <RosterPrivate.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef RUN_WITHOUT_REGISTRAR
	static BClipboard sClipboard(NULL);
	BClipboard *be_clipboard = &sClipboard;
#else
	BClipboard *be_clipboard = NULL;
#endif


using namespace BPrivate;


BClipboard::BClipboard(const char *name, bool transient)
	:
	fLock("clipboard")
{
	if (name != NULL)
		fName = strdup(name);
	else
		fName = strdup("system");

	fData = new BMessage();
	fCount = 0;

	BMessage message(B_REG_GET_CLIPBOARD_MESSENGER), reply;
	if (BRoster::Private().SendTo(&message, &reply, false) == B_OK
		&& reply.what == B_REG_SUCCESS
		&& reply.FindMessenger("messenger", &fClipHandler) == B_OK) {
		BMessage handlerMessage(B_REG_ADD_CLIPBOARD), handlerReply;
		int32 result;
		if (handlerMessage.AddString("name", fName) == B_OK
			&& fClipHandler.SendMessage(&handlerMessage, &handlerReply) == B_OK)
			handlerReply.FindInt32("result", &result);
	}
}


BClipboard::~BClipboard()
{
	free(fName);
	delete fData;
}


const char *
BClipboard::Name() const
{
	return (const char *)fName;
}


/*!	\brief Returns the (locally cached) number of commits to the clipboard.

	The returned value is the number of successful Commit() invocations for
	the clipboard represented by this object, either invoked on this object
	or another (even from another application). This method returns a locally
	cached value, which might already be obsolete. For an up-to-date value
	SystemCount() can be invoked.

	\return The number of commits to the clipboard.
*/
uint32
BClipboard::LocalCount() const
{
	return fCount;
}


/*!	\brief Returns the number of commits to the clipboard.

	The returned value is the number of successful Commit() invocations for
	the clipboard represented by this object, either invoked on this object
	or another (even from another application). This method retrieves the
	value directly from the system service managing the clipboards, so it is
	more expensive, but more up-to-date than LocalCount(), which returns a
	locally cached value.

	\return The number of commits to the clipboard.
*/
uint32
BClipboard::SystemCount() const
{
	int32 value;
	BMessage message(B_REG_GET_CLIPBOARD_COUNT), reply;
	if (message.AddString("name", fName) == B_OK
		&& fClipHandler.SendMessage(&message, &reply) == B_OK
		&& reply.FindInt32("count", &value) == B_OK)
		return (uint32)value;

	return 0;
}


status_t
BClipboard::StartWatching(BMessenger target)
{
	if (!target.IsValid())
		return B_BAD_VALUE;

	BMessage message(B_REG_CLIPBOARD_START_WATCHING), reply;
	if (message.AddString("name", fName) == B_OK
		&& message.AddMessenger("target", target) == B_OK
		&& fClipHandler.SendMessage(&message, &reply) == B_OK) {
		int32 result;
		reply.FindInt32("result", &result);
		return result;
	}
	return B_ERROR;
}


status_t
BClipboard::StopWatching(BMessenger target)
{
	if (!target.IsValid())
		return B_BAD_VALUE;

	BMessage message(B_REG_CLIPBOARD_STOP_WATCHING), reply;
	if (message.AddString("name", fName) == B_OK
		&& message.AddMessenger("target", target) == B_OK
		&& fClipHandler.SendMessage(&message, &reply) == B_OK) {
		int32 result;
		if (reply.FindInt32("result", &result) == B_OK)
			return result;
	}
	return B_ERROR;
}


bool
BClipboard::Lock()
{
	// Will this work correctly if clipboard is deleted while still waiting on
	// fLock.Lock() ?
	bool locked = fLock.Lock();

#ifndef RUN_WITHOUT_REGISTRAR
	if (locked && _DownloadFromSystem() != B_OK) {
		locked = false;
		fLock.Unlock();
	}
#endif

	return locked;
}


void
BClipboard::Unlock()
{
	fLock.Unlock();
}


bool
BClipboard::IsLocked() const
{
	return fLock.IsLocked();
}


status_t
BClipboard::Clear()
{
	if (!_AssertLocked())
		return B_NOT_ALLOWED;

	return fData->MakeEmpty();
}


status_t
BClipboard::Commit()
{
	return Commit(false);
}


status_t
BClipboard::Commit(bool failIfChanged)
{
	if (!_AssertLocked())
		return B_NOT_ALLOWED;

	status_t status = B_ERROR;
	BMessage message(B_REG_UPLOAD_CLIPBOARD), reply;
	if (message.AddString("name", fName) == B_OK
		&& message.AddMessage("data", fData) == B_OK
		&& message.AddMessenger("data source", be_app_messenger) == B_OK
		&& message.AddInt32("count", fCount) == B_OK
		&& message.AddBool("fail if changed", failIfChanged) == B_OK)
		status = fClipHandler.SendMessage(&message, &reply);

	if (status == B_OK) {
		int32 count;
		if (reply.FindInt32("count", &count) == B_OK)
			fCount = count;
	}

	return status;
}


status_t
BClipboard::Revert()
{
	if (!_AssertLocked())
		return B_NOT_ALLOWED;

	status_t status = fData->MakeEmpty();
	if (status == B_OK)
		status = _DownloadFromSystem();

	return status;
}


BMessenger
BClipboard::DataSource() const
{
	return fDataSource;
}


BMessage *
BClipboard::Data() const
{
	if (!_AssertLocked())
		return NULL;

    return fData;
}


//	#pragma mark - Private methods


BClipboard::BClipboard(const BClipboard &)
{
	// This is private, and I don't use it, so I'm not going to implement it
}


BClipboard & BClipboard::operator=(const BClipboard &)
{
	// This is private, and I don't use it, so I'm not going to implement it
	return *this;
}


void BClipboard::_ReservedClipboard1() {}
void BClipboard::_ReservedClipboard2() {}
void BClipboard::_ReservedClipboard3() {}


bool
BClipboard::_AssertLocked() const
{
	// This function is for jumping to the debugger if not locked
	if (!fLock.IsLocked()) {
		debugger("The clipboard must be locked before proceeding.");
		return false;
	}
	return true;
}


status_t
BClipboard::_DownloadFromSystem(bool force)
{
	// Apparently, the force paramater was used in some sort of
	// optimization in R5. Currently, we ignore it.
	BMessage message(B_REG_DOWNLOAD_CLIPBOARD), reply;
	if (message.AddString("name", fName) == B_OK
		&& fClipHandler.SendMessage(&message, &reply) == B_OK
		&& reply.FindMessage("data", fData) == B_OK
		&& reply.FindMessenger("data source", &fDataSource) == B_OK
		&& reply.FindInt32("count", (int32 *)&fCount) == B_OK)
		return B_OK;

	return B_ERROR;
}
