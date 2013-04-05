/*
 * Copyright 2011-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "IMAPConnectionWorker.h"

#include <Autolock.h>

#include "IMAPFolder.h"
#include "IMAPMailbox.h"
#include "IMAPProtocol.h"


IMAPConnectionWorker::IMAPConnectionWorker(IMAPProtocol& owner,
	const Settings& settings, bool main)
	:
	fOwner(owner),
	fSettings(settings),
	fIdleBox(NULL),
	fMain(main),
	fStopped(false)
{
}


IMAPConnectionWorker::~IMAPConnectionWorker()
{
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
	// TODO: we'll also need to interrupt listening to the socket
	fStopped = true;
}


status_t
IMAPConnectionWorker::_Worker()
{
	status_t status = fProtocol.Connect(fSettings.ServerAddress(),
		fSettings.Username(), fSettings.Password(), fSettings.UseSSL());
	if (status != B_OK)
		return status;

	bool idle = fSettings.IdleMode()
		&& fProtocol.Capabilities().Contains("IDLE");
	bool initial = true;

	while (!fStopped) {
		if (fMain) {
			// The main worker checks the subscribed folders, and creates
			// other workers as needed
			fOwner.CheckSubscribedFolders(fProtocol);
		}

		BAutolock locker(fLocker);

		if (!HasMailboxes()) {
			locker.Unlock();
			_Wait();
			continue;
		}

		if (!initial && idle && fIdleBox != NULL) {
			printf("%p: IDLE: %s\n", this, fIdleBox->MailboxName().String());
			// TODO: enter IDLE mode
		}

		MailboxMap::iterator iterator = fMailboxes.begin();
		for (; iterator != fMailboxes.end(); iterator++) {
			IMAPFolder* folder = iterator->first;
			if (!initial && idle && folder == fIdleBox)
				continue;

			printf("%p: check: %s\n", this, folder->MailboxName().String());
			IMAPMailbox* mailbox = iterator->second;
			if (mailbox == NULL) {
				mailbox = new IMAPMailbox(fProtocol, folder->MailboxName());
				folder->SetListener(mailbox);
			}

			IMAP::SelectCommand select(folder->MailboxName().String());
			status_t status = fProtocol.ProcessCommand(select);
			if (status == B_OK) {
				folder->SetUIDValidity(select.UIDValidity());
				// TODO: trigger download of mails until UIDNext()
			}
		}

		initial = false;
		// TODO: for now
		break;
	}

	return B_OK;
}


void
IMAPConnectionWorker::_Wait()
{
	while (acquire_sem(fOwner.FolderChangeSemaphore()) == B_INTERRUPTED);
}


/*static*/ status_t
IMAPConnectionWorker::_Worker(void* _self)
{
	IMAPConnectionWorker* self = (IMAPConnectionWorker*)_self;
	status_t status = self->_Worker();

	delete self;
	return status;
}
