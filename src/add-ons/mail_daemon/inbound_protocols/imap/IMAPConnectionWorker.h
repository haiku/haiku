/*
 * Copyright 2011-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_CONNECTION_WORKER_H
#define IMAP_CONNECTION_WORKER_H


#include <Locker.h>
#include <String.h>

#include "Protocol.h"


class IMAPProtocol;
class Settings;


class IMAPConnectionWorker {
public:
								IMAPConnectionWorker(IMAPProtocol& owner,
									const Settings& settings,
									bool main = false);
	virtual						~IMAPConnectionWorker();

			uint32				CountMailboxes() const;
			void				AddMailbox(const BString& name);
			void				RemoveMailbox(const BString& name);

			bool				IsMain() const { return fMain; }

			status_t			Start();
			void				Stop();

private:
			status_t			_Worker();
	static	status_t			_Worker(void* self);

private:
			IMAPProtocol&		fOwner;
			const Settings&		fSettings;
			IMAP::Protocol		fProtocol;
			BString				fIdleBox;
			StringList			fOtherBoxes;

			BLocker				fLocker;
			thread_id			fThread;
			bool				fMain;
			bool				fStopped;
};


#endif	// IMAP_CONNECTION_WORKER_H
