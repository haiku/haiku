/*
 * Copyright 2011-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "IMAPConnectionWorker.h"

#include <Autolock.h>

#include "IMAPFolder.h"
#include "IMAPMailbox.h"
#include "IMAPProtocol.h"


class WorkerPrivate {
public:
	WorkerPrivate(IMAPConnectionWorker& worker)
		:
		fWorker(worker)
	{
	}

	IMAP::Protocol& Protocol()
	{
		return fWorker.fProtocol;
	}

	void Quit()
	{
		fWorker.fStopped = true;
	}

private:
	IMAPConnectionWorker&	fWorker;
};


class WorkerCommand {
public:
								WorkerCommand() {}
	virtual						~WorkerCommand() {}

	virtual	status_t			Process(IMAPConnectionWorker& worker) = 0;
};


class QuitCommand : public WorkerCommand {
public:
	QuitCommand()
	{
	}

	virtual status_t Process(IMAPConnectionWorker& worker)
	{
		WorkerPrivate(worker).Quit();
		return B_OK;
	}
};


class CheckMailboxCommand : public WorkerCommand {
public:
	CheckMailboxCommand(IMAPFolder& folder, IMAPMailbox& mailbox)
		:
		fFolder(folder),
		fMailbox(mailbox)
	{
	}

	virtual status_t Process(IMAPConnectionWorker& worker)
	{
		IMAP::Protocol& protocol = WorkerPrivate(worker).Protocol();

		IMAP::SelectCommand select(fFolder.MailboxName().String());
		status_t status = protocol.ProcessCommand(select);
		if (status == B_OK) {
			fFolder.SetUIDValidity(select.UIDValidity());
			// TODO: trigger download of mails until UIDNext()
		}

		return B_OK;
	}

private:
	IMAPFolder&				fFolder;
	IMAPMailbox&			fMailbox;
};


// #pragma mark -


IMAPConnectionWorker::IMAPConnectionWorker(IMAPProtocol& owner,
	const Settings& settings, bool main)
	:
	fOwner(owner),
	fSettings(settings),
	fPendingCommandsSemaphore(-1),
	fIdleBox(NULL),
	fMain(main),
	fStopped(false)
{
	fExistsHandler.SetListener(this);
	fProtocol.AddHandler(fExistsHandler);
}


IMAPConnectionWorker::~IMAPConnectionWorker()
{
	delete_sem(fPendingCommandsSemaphore);
	_Disconnect();
}


bool
IMAPConnectionWorker::HasMailboxes() const
{
	BAutolock locker(const_cast<IMAPConnectionWorker*>(this)->fLocker);
	return !fMailboxes.empty();
}


uint32
IMAPConnectionWorker::CountMailboxes() const
{
	BAutolock locker(const_cast<IMAPConnectionWorker*>(this)->fLocker);
	return fMailboxes.size();
}


void
IMAPConnectionWorker::AddMailbox(IMAPFolder* folder)
{
	BAutolock locker(fLocker);

	fMailboxes.insert(std::make_pair(folder, (IMAPMailbox*)NULL));

	// Prefer to have the INBOX in idle mode over other mail boxes
	if (fIdleBox == NULL || folder->MailboxName().ICompare("INBOX") == 0)
		fIdleBox = folder;
}


void
IMAPConnectionWorker::RemoveAllMailboxes()
{
	BAutolock locker(fLocker);

	// Reset listeners, and delete the mailboxes
	MailboxMap::iterator iterator = fMailboxes.begin();
	for (; iterator != fMailboxes.end(); iterator++) {
		iterator->first->SetListener(NULL);
		delete iterator->second;
	}

	fIdleBox = NULL;
	fMailboxes.clear();
}


status_t
IMAPConnectionWorker::Run()
{
	fPendingCommandsSemaphore = create_sem(0, "imap pending commands");
	if (fPendingCommandsSemaphore < 0)
		return fPendingCommandsSemaphore;

	fThread = spawn_thread(&_Worker, "imap connection worker",
		B_NORMAL_PRIORITY, this);
	if (fThread < 0)
		return fThread;

	resume_thread(fThread);
	return B_OK;
}


void
IMAPConnectionWorker::Quit()
{
	_EnqueueCommand(new QuitCommand());
}


status_t
IMAPConnectionWorker::EnqueueCheckMailboxes()
{
	BAutolock locker(fLocker);

	MailboxMap::iterator iterator = fMailboxes.begin();
	for (; iterator != fMailboxes.end(); iterator++) {
		IMAPFolder* folder = iterator->first;

		printf("%p: check: %s\n", this, folder->MailboxName().String());
		IMAPMailbox* mailbox = iterator->second;
		if (mailbox == NULL) {
			mailbox = new IMAPMailbox(fProtocol, folder->MailboxName());
			folder->SetListener(mailbox);
		}

		status_t status = _EnqueueCommand(
			new CheckMailboxCommand(*folder, *mailbox));
		if (status != B_OK)
			return status;
	}
	return B_OK;
}


status_t
IMAPConnectionWorker::EnqueueRetrieveMail(entry_ref& ref)
{
	return B_OK;
}


void
IMAPConnectionWorker::MessageExistsReceived(uint32 index)
{
	printf("Message exists: %ld\n", index);
}


status_t
IMAPConnectionWorker::_Worker()
{
	while (!fStopped) {
		if (fMain) {
			// The main worker checks the subscribed folders, and creates
			// other workers as needed
			status_t status = _Connect();
			if (status == B_OK)
				status = fOwner.CheckSubscribedFolders(fProtocol, fIdle);
			if (status != B_OK)
				return status;
		}

		BAutolock locker(fLocker);

		if (fPendingCommands.IsEmpty()) {
			_Disconnect();
			locker.Unlock();

			_WaitForCommands();
			continue;
		}

		WorkerCommand* command = fPendingCommands.RemoveItemAt(0);
		if (command == NULL)
			continue;

		status_t status = _Connect();
		if (status != B_OK)
			return status;

		status = command->Process(*this);
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


/*!	Enqueues the given command to the worker queue. This method will take
	over ownership of the given command even in the error case.
*/
status_t
IMAPConnectionWorker::_EnqueueCommand(WorkerCommand* command)
{
	BAutolock locker(fLocker);

	if (!fPendingCommands.AddItem(command)) {
		delete command;
		return B_NO_MEMORY;
	}

	locker.Unlock();
	release_sem(fPendingCommandsSemaphore);
	return B_OK;
}


void
IMAPConnectionWorker::_WaitForCommands()
{
	while (acquire_sem(fPendingCommandsSemaphore) == B_INTERRUPTED);
}


status_t
IMAPConnectionWorker::_Connect()
{
	if (fProtocol.IsConnected())
		return B_OK;

	status_t status = fProtocol.Connect(fSettings.ServerAddress(),
		fSettings.Username(), fSettings.Password(), fSettings.UseSSL());
	if (status != B_OK)
		return status;

	fIdle = fSettings.IdleMode() && fProtocol.Capabilities().Contains("IDLE");
	return B_OK;
}


void
IMAPConnectionWorker::_Disconnect()
{
	fProtocol.Disconnect();
}


/*static*/ status_t
IMAPConnectionWorker::_Worker(void* _self)
{
	IMAPConnectionWorker* self = (IMAPConnectionWorker*)_self;
	status_t status = self->_Worker();

	delete self;
	return status;
}
