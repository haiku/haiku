/*
 * Copyright 2011-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_CONNECTION_WORKER_H
#define IMAP_CONNECTION_WORKER_H


#include <Locker.h>
#include <String.h>

#include "Protocol.h"


class IMAPFolder;
class IMAPMailbox;
class IMAPProtocol;
class Settings;


class IMAPConnectionWorker {
public:
								IMAPConnectionWorker(IMAPProtocol& owner,
									const Settings& settings,
									bool main = false);
	virtual						~IMAPConnectionWorker();

			bool				HasMailboxes() const;
			uint32				CountMailboxes() const;
			void				AddMailbox(IMAPFolder* folder);
			void				RemoveAllMailboxes();

			bool				IsMain() const { return fMain; }

			status_t			Run();
			void				Quit();

private:
			status_t			_Worker();
			void				_Wait();
	static	status_t			_Worker(void* self);

private:
	typedef std::map<IMAPFolder*, IMAPMailbox*> MailboxMap;

			IMAPProtocol&		fOwner;
			const Settings&		fSettings;
			IMAP::Protocol		fProtocol;
			IMAPFolder*			fIdleBox;
			MailboxMap			fMailboxes;

			BLocker				fLocker;
			thread_id			fThread;
			bool				fMain;
			bool				fStopped;
};


#endif	// IMAP_CONNECTION_WORKER_H
