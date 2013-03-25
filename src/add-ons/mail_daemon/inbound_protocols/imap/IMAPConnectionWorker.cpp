/*
 * Copyright 2011-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "IMAPConnectionWorker.h"

#include <Autolock.h>

#include "IMAPProtocol.h"


IMAPConnectionWorker::IMAPConnectionWorker(IMAPProtocol& owner,
	const Settings& settings, bool main)
	:
	fOwner(owner),
	fSettings(settings),
	fMain(main),
	fStopped(false)
{
}


IMAPConnectionWorker::~IMAPConnectionWorker()
{
}


uint32
IMAPConnectionWorker::CountMailboxes() const
{
	BAutolock locker(const_cast<IMAPConnectionWorker*>(this)->fLocker);
	return (fIdleBox.IsEmpty() ? 0 : 1) + fOtherBoxes.size();
}


void
IMAPConnectionWorker::AddMailbox(const BString& name)
{
	BAutolock locker(fLocker);

	if (fSettings.IdleMode() && fIdleBox.IsEmpty()) {
		fIdleBox = name;
	} else if (fSettings.IdleMode() && name == "INBOX") {
		// Prefer to have the INBOX in idle mode over other mail boxes
		fOtherBoxes.push_back(fIdleBox);
		fIdleBox = name;
	} else
		fOtherBoxes.push_back(name);
}


void
IMAPConnectionWorker::RemoveMailbox(const BString& name)
{
	BAutolock locker(fLocker);

	if (fSettings.IdleMode() && fIdleBox == name) {
		if (!fOtherBoxes.empty()) {
			fIdleBox = fOtherBoxes[0];
			fOtherBoxes.erase(fOtherBoxes.begin());
		} else
			fIdleBox.SetTo(NULL);
	} else {
		StringList::iterator iterator = fOtherBoxes.begin();
		for (; iterator != fOtherBoxes.end(); iterator++) {
			if (*iterator == name) {
				fOtherBoxes.erase(iterator);
				break;
			}
		}
	}
}


status_t
IMAPConnectionWorker::Start()
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

	while (!fStopped) {
		if (fMain) {
			// The main worker checks the subscribed folders, and creates
			// other workers as needed
			fOwner.CheckSubscribedFolders(fProtocol);
		}

		BAutolock locker(fLocker);

		if (!fIdleBox.IsEmpty())
			printf("%p: IDLE: %s\n", this, fIdleBox.String());

		StringList::iterator iterator = fOtherBoxes.begin();
		for (; iterator != fOtherBoxes.end(); iterator++) {
			printf("%p: check: %s\n", this, iterator->String());
		}

		// TODO: for now
		break;
	}

	return B_OK;
}


/*static*/ status_t
IMAPConnectionWorker::_Worker(void* _self)
{
	IMAPConnectionWorker* self = (IMAPConnectionWorker*)_self;
	status_t status = self->_Worker();

	delete self;
	return status;
}
