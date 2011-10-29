/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "MailDaemon.h"

#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <Beep.h>
#include <Catalog.h>
#include <Deskbar.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <fs_index.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Roster.h>
#include <StringList.h>
#include <VolumeRoster.h>

#include <E-mail.h>
#include <MailDaemon.h>
#include <MailMessage.h>
#include <MailSettings.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "MailDaemon"


using std::map;
using std::vector;


struct send_mails_info {
	send_mails_info()
	{
		totalSize = 0;
	}

	vector<entry_ref>	files;
	off_t				totalSize;
};


// #pragma mark -


static void
makeIndices()
{
	const char* stringIndices[] = {
		B_MAIL_ATTR_CC, B_MAIL_ATTR_FROM, B_MAIL_ATTR_NAME,
		B_MAIL_ATTR_PRIORITY, B_MAIL_ATTR_REPLY, B_MAIL_ATTR_STATUS,
		B_MAIL_ATTR_SUBJECT, B_MAIL_ATTR_TO, B_MAIL_ATTR_THREAD,
		B_MAIL_ATTR_ACCOUNT, NULL
	};

	// add mail indices for all devices capable of querying

	int32 cookie = 0;
	dev_t device;
	while ((device = next_dev(&cookie)) >= B_OK) {
		fs_info info;
		if (fs_stat_dev(device, &info) < 0
			|| (info.flags & B_FS_HAS_QUERY) == 0)
			continue;

		for (int32 i = 0; stringIndices[i]; i++)
			fs_create_index(device, stringIndices[i], B_STRING_TYPE, 0);

		fs_create_index(device, "MAIL:draft", B_INT32_TYPE, 0);
		fs_create_index(device, B_MAIL_ATTR_WHEN, B_INT32_TYPE, 0);
		fs_create_index(device, B_MAIL_ATTR_FLAGS, B_INT32_TYPE, 0);
		fs_create_index(device, B_MAIL_ATTR_ACCOUNT_ID, B_INT32_TYPE, 0);
		fs_create_index(device, B_MAIL_ATTR_READ, B_INT32_TYPE, 0);
	}
}


static void
addAttribute(BMessage& msg, const char* name, const char* publicName,
	int32 type = B_STRING_TYPE, bool viewable = true, bool editable = false,
	int32 width = 200)
{
	msg.AddString("attr:name", name);
	msg.AddString("attr:public_name", publicName);
	msg.AddInt32("attr:type", type);
	msg.AddBool("attr:viewable", viewable);
	msg.AddBool("attr:editable", editable);
	msg.AddInt32("attr:width", width);
	msg.AddInt32("attr:alignment", B_ALIGN_LEFT);
}


//	#pragma mark -


MailDaemonApp::MailDaemonApp()
	:
	BApplication(B_MAIL_DAEMON_SIGNATURE),

	fAutoCheckRunner(NULL)
{
	fErrorLogWindow = new ErrorLogWindow(BRect(200, 200, 500, 250),
		B_TRANSLATE("Mail daemon status log"), B_TITLED_WINDOW);
	fMailStatusWindow = new MailStatusWindow(BRect(40, 400, 360, 400),
		B_TRANSLATE("Mail Status"), fSettingsFile.ShowStatusWindow());
	// install MimeTypes, attributes, indices, and the
	// system beep add startup
	MakeMimeTypes();
	makeIndices();
	add_system_beep_event("New E-mail");
}


MailDaemonApp::~MailDaemonApp()
{
	delete fAutoCheckRunner;

	for (int32 i = 0; i < fQueries.CountItems(); i++)
		delete fQueries.ItemAt(i);

	delete fLEDAnimation;

	AccountMap::const_iterator it = fAccounts.begin();
	for (; it != fAccounts.end(); it++)
		_RemoveAccount(it);
}


void
MailDaemonApp::ReadyToRun()
{
	InstallDeskbarIcon();

	_InitAccounts();
	_UpdateAutoCheck(fSettingsFile.AutoCheckInterval());

	BVolume volume;
	BVolumeRoster roster;

	fNewMessages = 0;

	while (roster.GetNextVolume(&volume) == B_OK) {
		//{char name[255];volume.GetName(name);printf("Volume: %s\n",name);}

		BQuery* query = new BQuery;

		query->SetTarget(this);
		query->SetVolume(&volume);
		query->PushAttr(B_MAIL_ATTR_STATUS);
		query->PushString("New");
		query->PushOp(B_EQ);
		query->PushAttr("BEOS:TYPE");
		query->PushString("text/x-email");
		query->PushOp(B_EQ);
		query->PushAttr("BEOS:TYPE");
		query->PushString("text/x-partial-email");
		query->PushOp(B_EQ);
		query->PushOp(B_OR);
		query->PushOp(B_AND);
		query->Fetch();

		BEntry entry;
		while (query->GetNextEntry(&entry) == B_OK)
			fNewMessages++;

		fQueries.AddItem(query);
	}

	BString string, numString;
	if (fNewMessages > 0) {
		if (fNewMessages != 1)
			string << B_TRANSLATE("%num new messages.");
		else
			string << B_TRANSLATE("%num new message.");

		numString << fNewMessages;
		string.ReplaceFirst("%num", numString);
	}
	else
		string = B_TRANSLATE("No new messages");

	fCentralBeep = false;
	fMailStatusWindow->SetDefaultMessage(string);

	fLEDAnimation = new LEDAnimation;
	SetPulseRate(1000000);
}


void
MailDaemonApp::RefsReceived(BMessage* message)
{
	fMailStatusWindow->Activate(true);

	entry_ref ref;
	for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
		BNode node(&ref);
		if (node.InitCheck() < B_OK)
			continue;

		int32 account;
		if (node.ReadAttr(B_MAIL_ATTR_ACCOUNT_ID, B_INT32_TYPE, 0, &account,
			sizeof(account)) < 0)
			continue;

		InboundProtocolThread* protocolThread = _FindInboundProtocol(account);
		if (!protocolThread)
			continue;

		BMessenger target;
		BMessenger* messenger = &target;
		if (message->FindMessenger("target", &target) != B_OK)
			messenger = NULL;
		protocolThread->FetchBody(ref, messenger);
	}
}


void
MailDaemonApp::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case 'moto':
			if (fSettingsFile.CheckOnlyIfPPPUp()) {
				// TODO: check whether internet is up and running!
			}
			// supposed to fall through
		case kMsgCheckAndSend:	// check & send messages
			msg->what = kMsgSendMessages;
			PostMessage(msg);
			// supposed to fall trough
		case kMsgCheckMessage:	// check messages
			GetNewMessages(msg);
			break;

		case kMsgSendMessages:	// send messages
			SendPendingMessages(msg);
			break;

		case kMsgSettingsUpdated:
			fSettingsFile.Reload();
			_UpdateAutoCheck(fSettingsFile.AutoCheckInterval());
			fMailStatusWindow->SetShowCriterion(fSettingsFile.ShowStatusWindow());
			break;

		case kMsgAccountsChanged:
			_ReloadAccounts(msg);
			break;

		case kMsgSetStatusWindowMode:	// when to show the status window
		{
			int32 mode;
			if (msg->FindInt32("ShowStatusWindow", &mode) == B_OK)
				fMailStatusWindow->SetShowCriterion(mode);
			break;
		}

		case kMsgMarkMessageAsRead:
		{
			int32 account = msg->FindInt32("account");
			entry_ref ref;
			if (msg->FindRef("ref", &ref) != B_OK)
				break;
			read_flags read = (read_flags)msg->FindInt32("read");
			AccountMap::iterator it = fAccounts.find(account);
			if (it == fAccounts.end())
				break;
			InboundProtocolThread* inboundThread = it->second.inboundThread;
			inboundThread->MarkMessageAsRead(ref, read);
			break;
		}

		case kMsgFetchBody:
			RefsReceived(msg);
			break;

		case 'lkch':	// status window look changed
		case 'wsch':	// workspace changed
			fMailStatusWindow->PostMessage(msg);
			break;

		case 'stwg':	// Status window gone
		{
			BMessage reply('mnuc');
			reply.AddInt32("num_new_messages", fNewMessages);

			while ((msg = fFetchDoneRespondents.RemoveItemAt(0))) {
				msg->SendReply(&reply);
				delete msg;
			}

			if (fAlertString != B_EMPTY_STRING) {
				fAlertString.Truncate(fAlertString.Length() - 1);
				BAlert* alert = new BAlert(B_TRANSLATE("New Messages"),
					fAlertString.String(), "OK", NULL, NULL, B_WIDTH_AS_USUAL);
				alert->SetFeel(B_NORMAL_WINDOW_FEEL);
				alert->Go(NULL);
				fAlertString = B_EMPTY_STRING;
			}

			if (fCentralBeep) {
				system_beep("New E-mail");
				fCentralBeep = false;
			}
			break;
		}

		case 'mcbp':
			if (fNewMessages > 0)
				fCentralBeep = true;
			break;

		case kMsgCountNewMessages:	// Number of new messages
		{
			BMessage reply('mnuc');	// Mail New message Count
			if (msg->FindBool("wait_for_fetch_done")) {
				fFetchDoneRespondents.AddItem(DetachCurrentMessage());
				break;
			}

			reply.AddInt32("num_new_messages", fNewMessages);
			msg->SendReply(&reply);
			break;
		}

		case 'mblk':	// Mail Blink
			if (fNewMessages > 0)
				fLEDAnimation->Start();
			break;

		case 'enda':	// End Auto Check
			delete fAutoCheckRunner;
			fAutoCheckRunner = NULL;
			break;

		case 'numg':
		{
			int32 numMessages = msg->FindInt32("num_messages");
			BString numString;

			if (numMessages > 1)
				fAlertString << B_TRANSLATE("%num new messages for %name\n");
			else
				fAlertString << B_TRANSLATE("%num new message for %name\n");

			numString << numMessages;
			fAlertString.ReplaceFirst("%num", numString);
			fAlertString.ReplaceFirst("%name", msg->FindString("name"));
			break;
		}

		case B_QUERY_UPDATE:
		{
			int32 what;
			msg->FindInt32("opcode", &what);
			switch (what) {
				case B_ENTRY_CREATED:
					fNewMessages++;
					break;
				case B_ENTRY_REMOVED:
					fNewMessages--;
					break;
			}

			BString string, numString;

			if (fNewMessages > 0) {
				if (fNewMessages != 1)
					string << B_TRANSLATE("%num new messages.");
				else
					string << B_TRANSLATE("%num new message.");

			numString << fNewMessages;
			string.ReplaceFirst("%num", numString);
			}
			else
				string << B_TRANSLATE("No new messages.");

			fMailStatusWindow->SetDefaultMessage(string.String());
			break;
		}

		default:
			BApplication::MessageReceived(msg);
			break;
	}
}


void
MailDaemonApp::Pulse()
{
	bigtime_t idle = idle_time();
	if (fLEDAnimation->IsRunning() && idle < 100000)
		fLEDAnimation->Stop();
}


bool
MailDaemonApp::QuitRequested()
{
	RemoveDeskbarIcon();
	return true;
}


void
MailDaemonApp::InstallDeskbarIcon()
{
	BDeskbar deskbar;

	if (!deskbar.HasItem("mail_daemon")) {
		BRoster roster;
		entry_ref ref;

		status_t status = roster.FindApp(B_MAIL_DAEMON_SIGNATURE, &ref);
		if (status < B_OK) {
			fprintf(stderr, "Can't find application to tell deskbar: %s\n",
				strerror(status));
			return;
		}

		status = deskbar.AddItem(&ref);
		if (status < B_OK) {
			fprintf(stderr, "Can't add deskbar replicant: %s\n", strerror(status));
			return;
		}
	}
}


void
MailDaemonApp::RemoveDeskbarIcon()
{
	BDeskbar deskbar;
	if (deskbar.HasItem("mail_daemon"))
		deskbar.RemoveItem("mail_daemon");
}


void
MailDaemonApp::GetNewMessages(BMessage* msg)
{
	int32 account = -1;
	if (msg->FindInt32("account", &account) == B_OK && account >= 0) {
		InboundProtocolThread* protocol = _FindInboundProtocol(account);
		if (!protocol)
			return;
		protocol->SyncMessages();
		return;
	}

	// else check all accounts
	AccountMap::const_iterator it = fAccounts.begin();
	for (; it != fAccounts.end(); it++) {
		InboundProtocolThread* protocol = it->second.inboundThread;
		if (!protocol)
			continue;
		protocol->SyncMessages();
	}
}


void
MailDaemonApp::SendPendingMessages(BMessage* msg)
{
	BVolumeRoster roster;
	BVolume volume;

	map<int32, send_mails_info> messages;


	int32 account = -1;
	if (msg->FindInt32("account", &account) != B_OK)
		account = -1;

	if (!msg->HasString("message_path")) {
		while (roster.GetNextVolume(&volume) == B_OK) {
			BQuery query;
			query.SetVolume(&volume);
			query.PushAttr(B_MAIL_ATTR_FLAGS);
			query.PushInt32(B_MAIL_PENDING);
			query.PushOp(B_EQ);

			query.PushAttr(B_MAIL_ATTR_FLAGS);
			query.PushInt32(B_MAIL_PENDING | B_MAIL_SAVE);
			query.PushOp(B_EQ);

			if (account >= 0) {
				query.PushAttr(B_MAIL_ATTR_ACCOUNT_ID);
				query.PushInt32(account);
				query.PushOp(B_EQ);
				query.PushOp(B_AND);
			}

			query.PushOp(B_OR);
			query.Fetch();
			BEntry entry;
			while (query.GetNextEntry(&entry) == B_OK) {
				if (_IsEntryInTrash(entry))
					continue;

				BNode node;
				while (node.SetTo(&entry) == B_BUSY)
					snooze(1000);
				if (!_IsPending(node))
					continue;

				int32 messageAccount;
				if (node.ReadAttr(B_MAIL_ATTR_ACCOUNT_ID, B_INT32_TYPE, 0,
					&messageAccount, sizeof(int32)) < 0)
					messageAccount = -1;

				off_t size = 0;
				node.GetSize(&size);
				entry_ref ref;
				entry.GetRef(&ref);

				messages[messageAccount].files.push_back(ref);
				messages[messageAccount].totalSize += size;
			}
		}
	} else {
		const char* path;
		if (msg->FindString("message_path", &path) != B_OK)
			return;

		off_t size = 0;
		if (BNode(path).GetSize(&size) != B_OK)
			return;
		BEntry entry(path);
		entry_ref ref;
		entry.GetRef(&ref);

		messages[account].files.push_back(ref);
		messages[account].totalSize += size;
	}

	map<int32, send_mails_info>::iterator iter = messages.begin();
	for (; iter != messages.end(); iter++) {
		OutboundProtocolThread* protocolThread = _FindOutboundProtocol(
			iter->first);
		if (!protocolThread)
			continue;

		send_mails_info& info = iter->second;
		if (info.files.size() == 0)
			continue;

		MailProtocol* protocol = protocolThread->Protocol();

		protocolThread->Lock();
		protocol->SetTotalItems(info.files.size());
		protocol->SetTotalItemsSize(info.totalSize);
		protocolThread->Unlock();

		protocolThread->SendMessages(iter->second.files, info.totalSize);
	}
}


void
MailDaemonApp::MakeMimeTypes(bool remakeMIMETypes)
{
	// Add MIME database entries for the e-mail file types we handle.  Either
	// do a full rebuild from nothing, or just add on the new attributes that
	// we support which the regular BeOS mail daemon didn't have.

	const uint8 kNTypes = 2;
	const char* types[kNTypes] = {"text/x-email", "text/x-partial-email"};

	for (size_t i = 0; i < kNTypes; i++) {
		BMessage info;
		BMimeType mime(types[i]);
		if (mime.InitCheck() != B_OK) {
			fputs("could not init mime type.\n", stderr);
			return;
		}

		if (!mime.IsInstalled() || remakeMIMETypes) {
			// install the full mime type
			mime.Delete();
			mime.Install();

			// Set up the list of e-mail related attributes that Tracker will
			// let you display in columns for e-mail messages.
			addAttribute(info, B_MAIL_ATTR_NAME, "Name");
			addAttribute(info, B_MAIL_ATTR_SUBJECT, "Subject");
			addAttribute(info, B_MAIL_ATTR_TO, "To");
			addAttribute(info, B_MAIL_ATTR_CC, "Cc");
			addAttribute(info, B_MAIL_ATTR_FROM, "From");
			addAttribute(info, B_MAIL_ATTR_REPLY, "Reply To");
			addAttribute(info, B_MAIL_ATTR_STATUS, "Status");
			addAttribute(info, B_MAIL_ATTR_PRIORITY, "Priority", B_STRING_TYPE,
				true, true, 40);
			addAttribute(info, B_MAIL_ATTR_WHEN, "When", B_TIME_TYPE, true,
				false, 150);
			addAttribute(info, B_MAIL_ATTR_THREAD, "Thread");
			addAttribute(info, B_MAIL_ATTR_ACCOUNT, "Account", B_STRING_TYPE,
				true, false, 100);
			addAttribute(info, B_MAIL_ATTR_READ, "Read", B_INT32_TYPE,
				true, false, 70);
			mime.SetAttrInfo(&info);

			if (i == 0) {
				mime.SetShortDescription("E-mail");
				mime.SetLongDescription("Electronic Mail Message");
				mime.SetPreferredApp("application/x-vnd.Be-MAIL");
			} else {
				mime.SetShortDescription("Partial E-mail");
				mime.SetLongDescription("A Partially Downloaded E-mail");
				mime.SetPreferredApp("application/x-vnd.Be-MAIL");
			}
		}
	}
}


void
MailDaemonApp::_InitAccounts()
{
	BMailAccounts accounts;
	for (int i = 0; i < accounts.CountAccounts(); i++)
		_InitAccount(*accounts.AccountAt(i));
}


void
MailDaemonApp::_InitAccount(BMailAccountSettings& settings)
{
	account_protocols account;
	// inbound
	if (settings.IsInboundEnabled()) {
		account.inboundProtocol = _CreateInboundProtocol(settings,
			account.inboundImage);
	} else {
		account.inboundProtocol = NULL;
	}
	if (account.inboundProtocol) {
		DefaultNotifier* notifier = new DefaultNotifier(settings.Name(), true,
			fErrorLogWindow, fMailStatusWindow);
		account.inboundProtocol->SetMailNotifier(notifier);

		account.inboundThread = new InboundProtocolThread(
			account.inboundProtocol);
		account.inboundThread->Run();
	}

	// outbound
	if (settings.IsOutboundEnabled()) {
		account.outboundProtocol = _CreateOutboundProtocol(settings,
			account.outboundImage);
	} else {
		account.outboundProtocol = NULL;
	}
	if (account.outboundProtocol) {
		DefaultNotifier* notifier = new DefaultNotifier(settings.Name(), false,
			fErrorLogWindow, fMailStatusWindow);
		account.outboundProtocol->SetMailNotifier(notifier);

		account.outboundThread = new OutboundProtocolThread(
			account.outboundProtocol);
		account.outboundThread->Run();
	}

	printf("account name %s, id %i, in %p, out %p\n", settings.Name(),
		(int)settings.AccountID(), account.inboundProtocol,
		account.outboundProtocol);
	if (!account.inboundProtocol && !account.outboundProtocol)
		return;
	fAccounts[settings.AccountID()] = account;
}



void
MailDaemonApp::_ReloadAccounts(BMessage* message)
{
	type_code typeFound;
	int32 countFound;
	message->GetInfo("account", &typeFound, &countFound);
	if (typeFound != B_INT32_TYPE)
		return;

	// reload accounts
	BMailAccounts accounts;

	for (int i = 0; i < countFound; i++) {
		int32 account = message->FindInt32("account", i);
		AccountMap::const_iterator it = fAccounts.find(account);
		if (it != fAccounts.end())
			_RemoveAccount(it);
		BMailAccountSettings* settings = accounts.AccountByID(account);
		if (settings)
			_InitAccount(*settings);
	}
}


void
MailDaemonApp::_RemoveAccount(AccountMap::const_iterator it)
{
	BMessage reply;
	if (it->second.inboundThread) {
		it->second.inboundThread->SetStopNow();
		BMessenger(it->second.inboundThread).SendMessage(B_QUIT_REQUESTED,
			&reply);
	}
	if (it->second.outboundThread) {
		it->second.outboundThread->SetStopNow();
		BMessenger(it->second.outboundThread).SendMessage(B_QUIT_REQUESTED,
			&reply);
	}
	delete it->second.inboundProtocol;
	delete it->second.outboundProtocol;
	unload_add_on(it->second.inboundImage);
	unload_add_on(it->second.outboundImage);
	fAccounts.erase(it->first);
}


InboundProtocol*
MailDaemonApp::_CreateInboundProtocol(BMailAccountSettings& settings,
	image_id& image)
{
	const entry_ref& entry = settings.InboundPath();

	InboundProtocol* (*instantiate_protocol)(BMailAccountSettings*);

	BPath path(&entry);
	image = load_add_on(path.Path());
	if (image < 0)
		return NULL;
	if (get_image_symbol(image, "instantiate_inbound_protocol",
		B_SYMBOL_TYPE_TEXT, (void **)&instantiate_protocol) != B_OK) {
		unload_add_on(image);
		image = -1;
		return NULL;
	}
	InboundProtocol* protocol = (*instantiate_protocol)(&settings);
	return protocol;
}


OutboundProtocol*
MailDaemonApp::_CreateOutboundProtocol(BMailAccountSettings& settings,
	image_id& image)
{
	const entry_ref& entry = settings.OutboundPath();

	OutboundProtocol* (*instantiate_protocol)(BMailAccountSettings*);

	BPath path(&entry);
	image = load_add_on(path.Path());
	if (image < 0)
		return NULL;
	if (get_image_symbol(image, "instantiate_outbound_protocol",
		B_SYMBOL_TYPE_TEXT, (void **)&instantiate_protocol) != B_OK) {
		unload_add_on(image);
		image = -1;
		return NULL;
	}
	OutboundProtocol* protocol = (*instantiate_protocol)(&settings);
	return protocol;
}


InboundProtocolThread*
MailDaemonApp::_FindInboundProtocol(int32 account)
{
	AccountMap::iterator it = fAccounts.find(account);
	if (it == fAccounts.end())
		return NULL;
	return it->second.inboundThread;
}


OutboundProtocolThread*
MailDaemonApp::_FindOutboundProtocol(int32 account)
{
	if (account < 0)
		account = BMailSettings().DefaultOutboundAccount();

	AccountMap::iterator it = fAccounts.find(account);
	if (it == fAccounts.end())
		return NULL;
	return it->second.outboundThread;
}


void
MailDaemonApp::_UpdateAutoCheck(bigtime_t interval)
{
	if (interval > 0) {
		if (fAutoCheckRunner != NULL) {
			fAutoCheckRunner->SetInterval(interval);
			fAutoCheckRunner->SetCount(-1);
		} else
			fAutoCheckRunner = new BMessageRunner(be_app_messenger,
				new BMessage('moto'), interval);
	} else {
		delete fAutoCheckRunner;
		fAutoCheckRunner = NULL;
	}
}


/*!	Work-around for a broken index that contains out-of-date information.
*/
/*static*/ bool
MailDaemonApp::_IsPending(BNode& node)
{
	int32 flags;
	if (node.ReadAttr(B_MAIL_ATTR_FLAGS, B_INT32_TYPE, 0, &flags, sizeof(int32))
			!= (ssize_t)sizeof(int32))
		return false;

	return (flags & B_MAIL_PENDING) != 0;
}


/*static*/ bool
MailDaemonApp::_IsEntryInTrash(BEntry& entry)
{
	entry_ref ref;
	entry.GetRef(&ref);

	BVolume volume(ref.device);
	BPath path;
	if (volume.InitCheck() != B_OK
		|| find_directory(B_TRASH_DIRECTORY, &path, false, &volume) != B_OK)
		return false;

	BDirectory trash(path.Path());
	return trash.Contains(&entry);
}
