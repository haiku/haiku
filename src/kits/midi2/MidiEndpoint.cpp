/*
 * Copyright (c) 2002-2003 Matthijs Hollemans
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include "debug.h"
#include "MidiEndpoint.h"
#include "MidiRoster.h"
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
		str = name.String();
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

	if (name != newName) {
		BMessage msg;
		msg.AddString("midi:name", newName);

		if (SendChangeRequest(&msg) == B_OK) {
			if (LockLooper()) {
				name.SetTo(newName);
				UnlockLooper();
			}
		}
	}
}


int32
BMidiEndpoint::ID() const
{
	return id;
}


bool
BMidiEndpoint::IsProducer() const
{
	return !isConsumer;
}


bool
BMidiEndpoint::IsConsumer() const
{
	return isConsumer;
}


bool
BMidiEndpoint::IsRemote() const
{
	return !isLocal;
}


bool
BMidiEndpoint::IsLocal() const
{
	return isLocal;
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
	int32 old = atomic_add(&refCount, -1);

	TRACE(("BMidiEndpoint::Release refCount is now %ld", old - 1))

	if (old == 1) {
		// If the reference count of a local endpoint drops to zero, 
		// we must delete it. The destructor of BMidiLocalXXX calls
		// BMidiRoster::DeleteLocal(), which does all the hard work.
		// If we are a proxy for a remote endpoint, we must only be
		// deleted if that remote endpoint no longer exists.

		if (IsLocal() || !isAlive)
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
	atomic_add(&refCount, 1);

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
				*properties = *properties_;
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
		*_properties = *properties;
		UnlockLooper();
	}

	return B_OK;
}


status_t
BMidiEndpoint::Register(void)
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
BMidiEndpoint::Unregister(void)
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
		name.SetTo(name_);

	id           = 0;
	refCount     = 0;
	isLocal      = false;
	isRegistered = false;
	isAlive      = true;

	properties = new BMessage;
}


BMidiEndpoint::~BMidiEndpoint()
{
	TRACE(("BMidiEndpoint::~BMidiEndpoint (%p)", this))

	if (refCount > 0) {
		debugger(
			"you should use Release() to dispose of endpoints; "
			"do not \"delete\" them or allocate them on the stack!");
	}

	delete properties;
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
			isRegistered = registered; 
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

	return isRegistered;
}


bool
BMidiEndpoint::LockLooper() const
{
	return BMidiRoster::MidiRoster()->looper->Lock();
}


void
BMidiEndpoint::UnlockLooper() const
{
	BMidiRoster::MidiRoster()->looper->Unlock();
}

