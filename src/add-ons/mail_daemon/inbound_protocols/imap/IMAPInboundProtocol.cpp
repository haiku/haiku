/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "IMAPInboundProtocol.h"

#include <stdlib.h>

#include <Autolock.h>
#include <Directory.h>
#include <Messenger.h>
#include <NodeMonitor.h>

#include <crypt.h>


const uint32 kMsgStartWatching = '&StW';


DispatcherIMAPListener::DispatcherIMAPListener(MailProtocol& protocol,
	IMAPStorage& storage)
	:
	fProtocol(protocol),
	fStorage(storage)
{
}


void
DispatcherIMAPListener::HeaderFetched(int32 uid, BPositionIO* data,
	bool bodyIsComing)
{
	BFile* file = dynamic_cast<BFile*>(data);
	if (file == NULL)
		return;
	entry_ref ref;
	if (!fStorage.UIDToRef(uid, ref))
		return;

	fProtocol.NotifyHeaderFetched(ref, file);

	if (!bodyIsComing)
		fProtocol.ReportProgress(0, 1);
}


void
DispatcherIMAPListener::BodyFetched(int32 uid, BPositionIO* data)
{
	BFile* file = dynamic_cast<BFile*>(data);
	if (file == NULL)
		return;
	entry_ref ref;
	if (!fStorage.UIDToRef(uid, ref))
		return;
	fProtocol.NotifyBodyFetched(ref, file);

	fProtocol.ReportProgress(0, 1);
}


void
DispatcherIMAPListener::NewMessagesToFetch(int32 nMessages)
{
	fProtocol.NotifyNewMessagesToFetch(nMessages);
}


void
DispatcherIMAPListener::FetchEnd()
{
	fProtocol.ReportProgress(0, 1);
}


// #pragma mark -


IMAPMailboxThread::IMAPMailboxThread(IMAPInboundProtocol& protocol,
	IMAPMailbox& mailbox)
	:
	fProtocol(protocol),
	fIMAPMailbox(mailbox),

	fIsWatching(false),
	fThread(-1)
{
	fWatchSyncSem = create_sem(0, "watch sync sem");
}


IMAPMailboxThread::~IMAPMailboxThread()
{
	delete_sem(fWatchSyncSem);
}


bool
IMAPMailboxThread::IsWatching()
{
	BAutolock _(fLock);
	return fIsWatching;
}


status_t
IMAPMailboxThread::SyncAndStartWatchingMailbox()
{
	if (!fProtocol.IsConnected())
		return B_ERROR;

	if (fIMAPMailbox.SupportWatching()) {
		BAutolock autolock(fLock);
		if (fIsWatching)
			return B_OK;
		fThread = spawn_thread(_WatchThreadFunction, "IMAPMailboxThread",
			B_LOW_PRIORITY, this);
		if (resume_thread(fThread) != B_OK) {
			fThread = -1;
			return B_ERROR;
		}
		acquire_sem(fWatchSyncSem);
		fIsWatching = true;
	} else {
		status_t status = fIMAPMailbox.CheckMailbox();
		// if we lost connection reconnect and try again
		if (status != B_OK)
			status = fProtocol.Reconnect();
		if (status != B_OK)
			return status;
		status = fIMAPMailbox.CheckMailbox();
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


status_t
IMAPMailboxThread::StopWatchingMailbox()
{
	status_t status = fIMAPMailbox.StopWatchingMailbox();
	if (status != B_OK)
		return status;

	// wait till watching stopped
	status_t exitCode;
	return wait_for_thread(fThread, &exitCode);
}


/*static*/ status_t
IMAPMailboxThread::_WatchThreadFunction(void* data)
{
	((IMAPMailboxThread*)data)->_Watch();
	return B_OK;
}



void
IMAPMailboxThread::_Watch()
{
	status_t status = fIMAPMailbox.StartWatchingMailbox(fWatchSyncSem);
	if (status != B_OK)
		fProtocol.Disconnect();

	fLock.Lock();
	fIsWatching = false;
	fLock.Unlock();

	fThread = -1;
}


// #pragma mark -


MailboxWatcher::MailboxWatcher(IMAPInboundProtocol* protocol)
	:
	fProtocol(protocol)
{
}


MailboxWatcher::~MailboxWatcher()
{
	stop_watching(this);
}


void
MailboxWatcher::StartWatching(const char* mailboxDir)
{
	create_directory(mailboxDir, 0755);

	BDirectory dir(mailboxDir);
	dir.GetNodeRef(&fWatchDir);
	watch_node(&fWatchDir, B_WATCH_DIRECTORY, this);
}


void
MailboxWatcher::MessageReceived(BMessage* message)
{
	int32 opcode;
	entry_ref ref;
	node_ref nref;
	const char* name;
	switch (message->what) {
	case B_NODE_MONITOR:
		if (message->FindInt32("opcode", &opcode) != B_OK)
			break;
		switch (opcode) {
			case B_ENTRY_CREATED:
				break;
				message->FindInt32("device", &ref.device);
				message->FindInt64("directory", &ref.directory);
				message->FindString("name", &name);
				ref.set_name(name);

				fProtocol->AppendMessage(ref);
				break;

			case B_ENTRY_REMOVED:
				message->FindInt32("device", &nref.device);
				message->FindInt64("node", &nref.node);

				fProtocol->DeleteMessage(nref);
				break;

			case B_ENTRY_MOVED:
			{
				break;
				entry_ref from;
				entry_ref to;
				ino_t toDirectory = -1;
				ino_t fromDirectory = -2;
				message->FindInt64("to directory", &toDirectory);
				message->FindInt64("from directory", &fromDirectory);
				if (toDirectory != fromDirectory)
					break;
				message->FindInt32("device", &to.device);
				message->FindInt64("directory", &to.directory);
				message->FindString("name", &name);
				to.set_name(name);

				from.device = to.device;
				from.directory = to.directory;
				from.set_name(message->FindString("from name"));
				fProtocol->Looper()->TriggerFileRenamed(from, to);
				break;

				if (fWatchDir.node == toDirectory) {
					// append
					message->FindInt32("device", &ref.device);
					ref.directory = toDirectory;
					message->FindString("name", &name);
					ref.set_name(name);
					fProtocol->AppendMessage(ref);
				} else {
					// delete
					message->FindInt32("device", &nref.device);
					message->FindInt64("node", &nref.node);
					fProtocol->DeleteMessage(nref);
				}
				break;
			}
		}

	default:
		BHandler::MessageReceived(message);
	}
}


// #pragma mark -


IMAPInboundProtocol::IMAPInboundProtocol(BMailAccountSettings* settings,
	const char* mailbox)
	:
	InboundProtocol(settings),

	fIsConnected(false),
	fMailboxName(mailbox),
	fIMAPMailbox(fStorage),
	fDispatcherIMAPListener(*this, fStorage),
	fINBOXWatcher(NULL)
{
	const BMessage& settingsMsg = fAccountSettings.InboundSettings().Settings();
	int32 bodyLimit = 0;
	if (settingsMsg.HasInt32("partial_download_limit"))
		bodyLimit = settingsMsg.FindInt32("partial_download_limit");

	BString tempString;
	if (settingsMsg.FindString("destination", &tempString) != B_OK) {
		tempString = "/boot/home/mail/";
		tempString += settings->Name();
	}
	fMailboxPath.SetTo(tempString);
	fMailboxPath.Append(fMailboxName);
	fStorage.SetTo(fMailboxPath.Path());

	fIMAPMailbox.SetListener(&fDispatcherIMAPListener);
	fIMAPMailbox.SetFetchBodyLimit(bodyLimit);

	fIMAPMailboxThread = new IMAPMailboxThread(*this, fIMAPMailbox);

	// set watch directory
	fINBOXWatcher = new MailboxWatcher(this);
	AddHandler(fINBOXWatcher);
}


IMAPInboundProtocol::~IMAPInboundProtocol()
{
	fIMAPMailboxThread->StopWatchingMailbox();

	RemoveHandler(fINBOXWatcher);
	delete fINBOXWatcher;

	delete fIMAPMailboxThread;
}


status_t
IMAPInboundProtocol::Connect(const char* server, const char* username,
	const char* password, bool useSSL, int32 port)
{
	if (fIsConnected)
		return B_OK;

	BString statusMessage;

	statusMessage = "Connect to: ";
	statusMessage += username;
	SetTotalItems(5);
	ReportProgress(0, 1, statusMessage);

	status_t status = fIMAPMailbox.Connect(server, username, password, useSSL,
		port);
	if (status != B_OK) {
		statusMessage = "Failed to login: ";
		statusMessage += fIMAPMailbox.CommandError();
		ShowError(statusMessage);
		ResetProgress();
		return status;
	}

	ReportProgress(0, 1, "Load database");

	fStorage.StartReadDatabase();
	status = fIMAPMailbox.SelectMailbox(fMailboxName);
	if (status != B_OK) {
		fStorage.WaitForDatabaseRead();
		statusMessage = "Failed to select mailbox (";
		statusMessage += fMailboxName;
		statusMessage += "): ";
		statusMessage += fIMAPMailbox.CommandError();
		ShowError(statusMessage);
		ResetProgress();
		return status;
	}

	ReportProgress(0, 1, "Fetch message list");
	status = fIMAPMailbox.Sync();
	if (status != B_OK) {
		fStorage.WaitForDatabaseRead();
		ShowError("Failed to sync mailbox");
		ResetProgress();
		return status;
	}

	ReportProgress(0, 1, "Read local message list");
	status = fStorage.WaitForDatabaseRead();
	if (status != B_OK) {
		ShowError("Can't read database");
		ResetProgress();
		return status;
	}

	ReportProgress(0, 1, "Sync mailbox");

	IMAPMailboxSync mailboxSync;
	status = mailboxSync.Sync(fStorage, fIMAPMailbox);

	ResetProgress();

	if (status == B_OK)
		fIsConnected = true;
	else
		Disconnect();
	return status;
}


status_t
IMAPInboundProtocol::Disconnect()
{
	fIsConnected = false;
	return fIMAPMailbox.Disconnect();
}


status_t
IMAPInboundProtocol::Reconnect()
{
	Disconnect();
	return Connect(fServer, fUsername, fPassword, fUseSSL, -1);
}


bool
IMAPInboundProtocol::IsConnected()
{
	if (!fIsConnected)
		return false;
	if (fIMAPMailbox.IsConnected())
		return true;

	Disconnect();
	return false;
}


void
IMAPInboundProtocol::SetStopNow()
{
	fIMAPMailbox.SetStopNow();
}


void
IMAPInboundProtocol::AddedToLooper()
{
	fINBOXWatcher->StartWatching(fMailboxPath.Path());

	const BMessage& settingsMsg = fAccountSettings.InboundSettings().Settings();
	UpdateSettings(settingsMsg);
}


void
IMAPInboundProtocol::UpdateSettings(const BMessage& settings)
{
	settings.FindString("server", &fServer);
	int32 ssl;
	settings.FindInt32("flavor", &ssl);
	if (ssl == 1)
		fUseSSL = true;
	else
		fUseSSL = false;
	settings.FindString("username", &fUsername);

	char* passwd = get_passwd(&settings, "cpasswd");
	if (passwd) {
		fPassword = passwd;
		delete[] passwd;
	}

	SyncMessages();
}


bool
IMAPInboundProtocol::InterestingEntry(const entry_ref& ref)
{
	BEntry entry(&ref);
	return BDirectory(fMailboxPath.Path()).Contains(&entry, B_FILE_NODE);
}


status_t
IMAPInboundProtocol::SyncMessages()
{
	if (!IsConnected())
		Connect(fServer, fUsername, fPassword, fUseSSL);

	return fIMAPMailboxThread->SyncAndStartWatchingMailbox();
}


status_t
IMAPInboundProtocol::FetchBody(const entry_ref& ref)
{
	if (!IsConnected())
		Connect(fServer, fUsername, fPassword, fUseSSL);

	fIMAPMailboxThread->StopWatchingMailbox();
	int32 uid = fStorage.RefToUID(ref);

	ResetProgress("Fetch body");
	SetTotalItems(1);

	status_t status = fIMAPMailbox.FetchBody(fIMAPMailbox.UIDToMessageNumber(
		uid));

	ResetProgress();

	fIMAPMailboxThread->SyncAndStartWatchingMailbox();
	return status;
}


status_t
IMAPInboundProtocol::MarkMessageAsRead(const entry_ref& ref, read_flags flag)
{
	if (!IsConnected())
		Connect(fServer, fUsername, fPassword, fUseSSL);

	fIMAPMailboxThread->StopWatchingMailbox();
	int32 uid = fStorage.RefToUID(ref);
	int32 flags = fStorage.GetFlags(uid);
	if (flag == B_READ)
		flags |= kSeen;
	else
		flags &= ~kSeen;
	status_t status = fIMAPMailbox.SetFlags(
		fIMAPMailbox.UIDToMessageNumber(uid), flags);
	fIMAPMailboxThread->SyncAndStartWatchingMailbox();
	if (status != B_OK)
		return status;
	return InboundProtocol::MarkMessageAsRead(ref, flag);
}


status_t
IMAPInboundProtocol::DeleteMessage(const entry_ref& ref)
{
	if (!IsConnected())
		Connect(fServer, fUsername, fPassword, fUseSSL);

	fIMAPMailboxThread->StopWatchingMailbox();

	int32 uid = fStorage.RefToUID(ref);
	status_t status = fIMAPMailbox.DeleteMessage(uid, false);

	fIMAPMailboxThread->SyncAndStartWatchingMailbox();
	return status;
}


void
IMAPInboundProtocol::FileRenamed(const entry_ref& from, const entry_ref& to)
{
	fStorage.FileRenamed(from, to);
}


void
IMAPInboundProtocol::FileDeleted(const node_ref& node)
{
	//TODO
}


status_t
IMAPInboundProtocol::AppendMessage(const entry_ref& ref)
{
	if (!IsConnected())
		Connect(fServer, fUsername, fPassword, fUseSSL);

	fIMAPMailboxThread->StopWatchingMailbox();
	BFile file(&ref, B_READ_ONLY);
	off_t size;
	file.GetSize(&size);

	// check if file is already known
	int32 uid = -1;
	file.ReadAttr("MAIL:unique_id", B_INT32_TYPE, 0, &uid, sizeof(int32));
	if (fStorage.HasFile(uid)) {
		fIMAPMailboxThread->SyncAndStartWatchingMailbox();
		return B_BAD_VALUE;
	}

	int32 flags = 0;
	file.ReadAttr("MAIL:server_flags", B_INT32_TYPE, 0, &flags, sizeof(int32));

	status_t status = fIMAPMailbox.AppendMessage(file, size, flags);
	fIMAPMailboxThread->SyncAndStartWatchingMailbox();
	return status;
}


status_t
IMAPInboundProtocol::DeleteMessage(node_ref& node)
{
	if (!IsConnected())
		Connect(fServer, fUsername, fPassword, fUseSSL);

	fIMAPMailboxThread->StopWatchingMailbox();
	StorageMailEntry* entry = fStorage.GetEntryForRef(node);
	if (entry == NULL) {
		fIMAPMailboxThread->SyncAndStartWatchingMailbox();
		return B_BAD_VALUE;
	}

	int32 uid = entry->uid;
	status_t status = fIMAPMailbox.DeleteMessage(uid, false);

	fIMAPMailboxThread->SyncAndStartWatchingMailbox();
	return status;
}
