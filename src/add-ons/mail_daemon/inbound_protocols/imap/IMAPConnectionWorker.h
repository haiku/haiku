/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_CONNECTION_WORKER_H
#define IMAP_CONNECTION_WORKER_H


#include <Locker.h>
#include <String.h>

#include "Protocol.h"


class IMAPConnectionWorker {
public:
								IMAPConnectionWorker(IMAP::Protocol& protocol,
									StringList& mailboxes);
	virtual						~IMAPConnectionWorker();

			status_t			Start(bool usePush);
			void				Stop();

private:
			status_t			_Worker();
	static	status_t			_Worker(void* self);

private:
			IMAP::Protocol&		fProtocol;
			BString				fIdleBox;
			StringList			fOtherBoxes;

			BLocker				fLocker;
			thread_id			fThread;
};


#endif	// IMAP_CONNECTION_WORKER_H
