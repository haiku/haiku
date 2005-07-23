/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "MessageLooper.h"

#include <Autolock.h>


MessageLooper::MessageLooper(const char* name)
	: BLocker(name),
	fQuitting(false)
{
}


MessageLooper::~MessageLooper()
{
}


bool
MessageLooper::Run()
{
	BAutolock locker(this);

	fQuitting = false;

	char name[B_OS_NAME_LENGTH];
	_GetLooperName(name, sizeof(name));

	// Spawn our message-monitoring thread
	fThread = spawn_thread(_message_thread, name, B_NORMAL_PRIORITY, this);
	if (fThread < B_OK) {
		fQuitting = true;
		return false;
	}

	if (resume_thread(fThread) != B_OK) {
		fQuitting = true;
		kill_thread(fThread);
		fThread = -1;
		return false;
	}

	return true;
}


void
MessageLooper::Quit()
{
}


/*!
	\brief Send a message to the looper without any attachments
	\param code ID code of the message to post
*/
void
MessageLooper::PostMessage(int32 code)
{
	BPrivate::LinkSender link(_MessagePort());
	link.StartMessage(code);
	link.Flush();
}


void
MessageLooper::_GetLooperName(char* name, size_t length)
{
	strcpy(name, "unnamed looper");
}


void
MessageLooper::_DispatchMessage(int32 code, BPrivate::LinkReceiver &link)
{
}


void
MessageLooper::_MessageLooper()
{
}


/*!
	\brief Message-dispatching loop starter
*/
/*static*/
int32
MessageLooper::_message_thread(void* _looper)
{
	MessageLooper* looper = (MessageLooper*)_looper;

	looper->_MessageLooper();
	return 0;
}

