/*
 * Copyright 2006, Haiku.
 *
 * Copyright (c) 2002-2003 Matthijs Hollemans
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Matthijs Hollemans
 */

#include "debug.h"
#include <MidiEndpoint.h>
#include <MidiRoster.h>
#include "MidiRosterLooper.h"
#include "protocol.h"


const char*
BMidiEndpoint::Name() const
{
	const char* str = NULL;

	// It seems reasonable to assume that the pointer returned by
	// BString::String() can change when the string is modified,
	// e.g. to allocate more space. That's why we lock here too.

	if (LockLooper()) {
		str = fName.String();
		UnlockLooper();
	}

	return str;
}


void
BMidiEndpoint::SetName(const char* newName)
{
	if (newName == NULL) {
		WARN("SetName() does not accept a NULL name");
		return;
	}
	if (IsRemote()) {
		WARN("SetName() is not allowed on remote endpoints");
		return;
	}
	if (!IsValid())
		return;

	if (fName != newName) {
		BMessage msg;
		msg.AddString("midi:name", newName);

		if (SendChangeRequest(&msg) == B_OK) {
			if (LockLooper()) {
				fName.SetTo(newName);
				UnlockLooper();
			}
		}
	}
}


int32
BMidiEndpoint::ID() const
{
	return fId;
}


bool
BMidiEndpoint::IsProducer() const
{
	return !fIsConsumer;
}


bool
BMidiEndpoint::IsConsumer() const
{
	return fIsConsumer;
}


bool
BMidiEndpoint::IsRemote() const
{
	return !fIsLocal;
}


bool
BMidiEndpoint::IsLocal() const
{
	return fIsLocal;
}


bool
BMidiEndpoint::IsPersistent() const
{
	return false;
}


bool
BMidiEndpoint::IsValid() const
{
	if (IsLocal())
		return (ID() > 0);

	// remote endpoint
	return IsRegistered();
}


status_t
BMidiEndpoint::Release()
{
	int32 old = atomic_add(&fRefCount, -1);

	TRACE(("BMidiEndpoint::Release refCount is now %ld", old - 1))

	if (old == 1) {
		// If the reference count of a local endpoint drops to zero, 
		// we must delete it. The destructor of BMidiLocalXXX calls
		// BMidiRoster::DeleteLocal(), which does all the hard work.
		// If we are a proxy for a remote endpoint, we must only be
		// deleted if that remote endpoint no longer exists.

		if (IsLocal() || !fIsAlive)
			delete this;
	} else if (old <= 0) {
		debugger("too many calls to Release()");
	}

	return B_OK;
}


status_t
BMidiEndpoint::Acquire()
{
#ifdef DEBUG
	int32 old = 
#endif
	atomic_add(&fRefCount, 1);

	TRACE(("BMidiEndpoint::Acquire refCount is now %ld", old + 1))

	return B_OK;
}


status_t
BMidiEndpoint::SetProperties(const BMessage* properties_)
{
	if (properties_ == NULL) {
		WARN("SetProperties() does not accept a NULL message")
		return B_BAD_VALUE;
	} else if (IsRemote()) {
		WARN("SetProperties() is not allowed on remote endpoints");
		return B_ERROR;
	} else if (!IsValid())  {
		return B_ERROR;
	} else {
		BMessage msg;
		msg.AddMessage("midi:properties", properties_);

		status_t err = SendChangeRequest(&msg);
		if (err == B_OK) {
			if (LockLooper()) {
				*fProperties = *properties_;
				UnlockLooper();
			}
		}

		return err;
	}
}


status_t
BMidiEndpoint::GetProperties(BMessage* _properties) const
{
	if (_properties == NULL) {
		WARN("GetProperties() does not accept NULL properties")
		return B_BAD_VALUE;
	}

	if (LockLooper()) {
		*_properties = *fProperties;
		UnlockLooper();
	}

	return B_OK;
}


status_t
BMidiEndpoint::Register()
{
	if (IsRemote()) { 
		WARN("You cannot Register() remote endpoints");
		return B_ERROR; 
	}
	if (IsRegistered()) { 
		WARN("This endpoint is already registered");
		return B_OK; 
	}
	if (!IsValid()) 
		return B_ERROR;

	return SendRegisterRequest(true);
}


status_t
BMidiEndpoint::Unregister()
{
	if (IsRemote()) { 
		WARN("You cannot Unregister() remote endpoints");
		return B_ERROR; 
	}
	if (!IsRegistered()) { 
		WARN("This endpoint is already unregistered");
		return B_OK; 
	}
	if (!IsValid())
		return B_ERROR;

	return SendRegisterRequest(false);
}


BMidiEndpoint::BMidiEndpoint(const char* name_)
{
	TRACE(("BMidiEndpoint::BMidiEndpoint"))

	if (name_ != NULL)
		fName.SetTo(name_);

	fId           = 0;
	fRefCount     = 0;
	fIsLocal      = false;
	fIsRegistered = false;
	fIsAlive      = true;
	
	fProperties	= new BMessage();
}


BMidiEndpoint::~BMidiEndpoint()
{
	TRACE(("BMidiEndpoint::~BMidiEndpoint (%p)", this))

	if (fRefCount > 0) {
		debugger(
			"you should use Release() to dispose of endpoints; "
			"do not \"delete\" them or allocate them on the stack!");
	}
	delete fProperties;
}

//------------------------------------------------------------------------------

void BMidiEndpoint::_Reserved1() { }
void BMidiEndpoint::_Reserved2() { }
void BMidiEndpoint::_Reserved3() { }
void BMidiEndpoint::_Reserved4() { }
void BMidiEndpoint::_Reserved5() { }
void BMidiEndpoint::_Reserved6() { }
void BMidiEndpoint::_Reserved7() { }
void BMidiEndpoint::_Reserved8() { }

//------------------------------------------------------------------------------

status_t
BMidiEndpoint::SendRegisterRequest(bool registered)
{
	BMessage msg;
	msg.AddBool("midi:registered", registered);

	status_t err = SendChangeRequest(&msg);
	if (err == B_OK)  {
		if (LockLooper()) {
			fIsRegistered = registered; 
			UnlockLooper();
		}
	}

	return err;
}


status_t
BMidiEndpoint::SendChangeRequest(BMessage* msg)
{
	ASSERT(msg != NULL)

	msg->what = MSG_CHANGE_ENDPOINT;
	msg->AddInt32("midi:id", ID());

	BMessage reply;
	status_t err = BMidiRoster::MidiRoster()->SendRequest(msg, &reply);
	if (err != B_OK)
		return err;

	status_t res;
	if (reply.FindInt32("midi:result", &res) == B_OK)
		return res;

	return B_ERROR;
}


bool
BMidiEndpoint::IsRegistered() const
{
	// No need to protect this with a lock, because reading
	// and writing a bool is always an atomic operation.

	return fIsRegistered;
}


bool
BMidiEndpoint::LockLooper() const
{
	return BMidiRoster::MidiRoster()->fLooper->Lock();
}


void
BMidiEndpoint::UnlockLooper() const
{
	BMidiRoster::MidiRoster()->fLooper->Unlock();
}

