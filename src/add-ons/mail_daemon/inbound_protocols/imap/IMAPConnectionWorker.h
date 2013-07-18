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


class IMAPConnectionWorker : public IMAP::ExistsListener,
	IMAP::ExpungeListener {
public:
								IMAPConnectionWorker(IMAPProtocol& owner,
									const Settings& settings,
									bool main = false);
	virtual						~IMAPConnectionWorker();

			bool				HasMailboxes() const;
			uint32				CountMailboxes() const;
			void				AddMailbox(IMAPFolder* folder);
			void				RemoveAllMailboxes();

			IMAPProtocol&		Owner() const { return fOwner; }
			bool				IsMain() const { return fMain; }
			bool				UsesIdle() const { return fIdle; }

			status_t			Run();
			void				Quit();

			status_t			EnqueueCheckSubscribedFolders();
			status_t			EnqueueCheckMailboxes();
			status_t			EnqueueRetrieveMail(entry_ref& ref);

	// Handler listener
	virtual	void				MessageExistsReceived(uint32 index);
	virtual	void				MessageExpungeReceived(uint32 index);

private:
			status_t			_Worker();
			status_t			_EnqueueCommand(WorkerCommand* command);
			void				_WaitForCommands();

			status_t			_SelectMailbox(IMAPFolder& folder,
									uint32* _nextUID);
			IMAPMailbox*		_MailboxFor(IMAPFolder& folder);
			IMAPFolder*			_Selected() const { return fSelectedBox; }
			void				_SyncCommandDone();
			uint32				_MessagesExist() const
									{ return fMessagesExist; }

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
			uint32				fSyncPending;

			IMAP::ExistsHandler	fExistsHandler;
			IMAP::ExpungeHandler fExpungeHandler;

			IMAPFolder*			fIdleBox;
			IMAPFolder*			fSelectedBox;
			MailboxMap			fMailboxes;
			uint32				fMessagesExist;

			BLocker				fLocker;
			thread_id			fThread;
			bool				fMain;
			bool				fStopped;
			bool				fIdle;
};


#endif	// IMAP_CONNECTION_WORKER_H
