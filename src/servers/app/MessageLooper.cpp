/*
 * Copyright 2005-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "MessageLooper.h"

#include <stdio.h>
#include <string.h>

#include <Autolock.h>


MessageLooper::MessageLooper(const char* name)
	:
	BLocker(name),
	fThread(-1),
	fQuitting(false),
	fDeathSemaphore(-1)
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
	fThread = spawn_thread(_message_thread, name, B_DISPLAY_PRIORITY, this);
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
	fQuitting = true;
	_PrepareQuit();

	if (fThread < B_OK) {
		// thread has not been started yet
		delete this;
		return;
	}

	if (fThread == find_thread(NULL)) {
		// called from our message looper
		delete this;
		exit_thread(0);
	} else {
		// called from a different thread
		PostMessage(kMsgQuitLooper);
	}
}


/*!
	\brief Send a message to the looper without any attachments
	\param code ID code of the message to post
*/
status_t
MessageLooper::PostMessage(int32 code, bigtime_t timeout)
{
	BPrivate::LinkSender link(MessagePort());
	link.StartMessage(code);
	return link.Flush(timeout);
}


/*static*/
status_t
MessageLooper::WaitForQuit(sem_id semaphore, bigtime_t timeout)
{
	status_t status;
	do {
		status = acquire_sem_etc(semaphore, 1, B_RELATIVE_TIMEOUT, timeout);
	} while (status == B_INTERRUPTED);

	if (status == B_TIMED_OUT)
		return status;

	return B_OK;
}


void
MessageLooper::_PrepareQuit()
{
	// to be implemented by subclasses
}


void
MessageLooper::_GetLooperName(char* name, size_t length)
{
	sem_id semaphore = Sem();
	sem_info info;
	if (get_sem_info(semaphore, &info) == B_OK)
		strlcpy(name, info.name, length);
	else
		strlcpy(name, "unnamed looper", length);
}


void
MessageLooper::_DispatchMessage(int32 code, BPrivate::LinkReceiver &link)
{
}


void
MessageLooper::_MessageLooper()
{
	BPrivate::LinkReceiver& receiver = fLink.Receiver();

	while (true) {
		int32 code;
		status_t status = receiver.GetNextMessage(code);
		if (status < B_OK) {
			// that shouldn't happen, it's our port
			char name[256];
			_GetLooperName(name, 256);
			printf("MessageLooper \"%s\": Someone deleted our message port %ld, %s!\n",
				name, receiver.Port(), strerror(status));
			break;
		}

		Lock();

		if (code == kMsgQuitLooper) {
			Quit();
		} else
			_DispatchMessage(code, receiver);

		Unlock();
	}
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

