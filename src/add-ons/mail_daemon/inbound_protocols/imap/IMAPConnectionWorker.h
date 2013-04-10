/*
 * Copyright 2011-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_CONNECTION_WORKER_H
#define IMAP_CONNECTION_WORKER_H


#include <Locker.h>
#include <String.h>

//#include "Commands.h"
#include "Protocol.h"


class IMAPFolder;
class IMAPMailbox;
class IMAPProtocol;
class Settings;
class WorkerCommand;
class WorkerPrivate;


typedef BObjectList<WorkerCommand> WorkerCommandList;


class IMAPConnectionWorker : public IMAP::ExistsListener {
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

			status_t			EnqueueCheckMailboxes();
			status_t			EnqueueRetrieveMail(entry_ref& ref);

	// Handler listener
	virtual	void				MessageExistsReceived(uint32 index);

private:
			status_t			_Worker();
			status_t			_EnqueueCommand(WorkerCommand* command);
			void				_WaitForCommands();
			status_t			_Connect();
			void				_Disconnect();
	static	status_t			_Worker(void* self);

private:
	typedef std::map<IMAPFolder*, IMAPMailbox*> MailboxMap;
	friend class WorkerPrivate;

			IMAPProtocol&		fOwner;
			const Settings&		fSettings;
			IMAP::Protocol		fProtocol;
			sem_id				fPendingCommandsSemaphore;
			WorkerCommandList	fPendingCommands;

			IMAP::ExistsHandler	fExistsHandler;

			IMAPFolder*			fIdleBox;
			MailboxMap			fMailboxes;

			BLocker				fLocker;
			thread_id			fThread;
			bool				fMain;
			bool				fStopped;
			bool				fIdle;
};


#endif	// IMAP_CONNECTION_WORKER_H
