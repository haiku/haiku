/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "IMAPConnectionWorker.h"


IMAPConnectionWorker::IMAPConnectionWorker(IMAP::Protocol& protocol,
	StringList& mailboxes)
	:
	fProtocol(protocol)
{
}


IMAPConnectionWorker::~IMAPConnectionWorker()
{
}


status_t
IMAPConnectionWorker::Start(bool usePush)
{
	fThread = spawn_thread(&_Worker, "imap connection worker",
		B_NORMAL_PRIORITY, this);
	if (fThread < 0)
		return fThread;

	resume_thread(fThread);
	return B_OK;
}


void
IMAPConnectionWorker::Stop()
{
}


status_t
IMAPConnectionWorker::_Worker()
{
	return B_OK;
}


/*static*/ status_t
IMAPConnectionWorker::_Worker(void* self)
{
	return ((IMAPConnectionWorker*)self)->_Worker();
}
