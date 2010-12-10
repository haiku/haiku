/*
 * Copyright 2009-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


//!	The daemon's inner workings


#include <Application.h>
#include <Beep.h>
#include <Button.h>
#include <ChainRunner.h>
#include <Deskbar.h>
#include <File.h>
#include <FindDirectory.h>
#include <fs_index.h>
#include <fs_info.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Mime.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Query.h>
#include <Roster.h>
#include <String.h>
#include <StringView.h>
#include <VolumeRoster.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <map>

#include <E-mail.h>
#include <MailSettings.h>
#include <MailMessage.h>
#include <status.h>
#include <StringList.h>

#include "DeskbarView.h"
#include "LEDAnimation.h"

#include <MDRLanguage.h>


using std::map;

typedef struct glorbal {
	size_t bytes;
	BStringList msgs;
} snuzzwut;


static BMailStatusWindow* sStatus;


class MailDaemonApp : public BApplication {
public:
								MailDaemonApp();
	virtual						~MailDaemonApp();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				RefsReceived(BMessage* message);

	virtual void				Pulse();
	virtual bool				QuitRequested();
	virtual void				ReadyToRun();

			void				InstallDeskbarIcon();
			void				RemoveDeskbarIcon();

			void				RunChains(BList& list, BMessage* message);
			void				SendPendingMessages(BMessage* message);
			void				GetNewMessages(BMessage* message);

private:
			void				_UpdateAutoCheck(bigtime_t interval);
	static	bool				_IsPending(BNode& node);
	static	bool				_IsEntryInTrash(BEntry& entry);

private:
			BMessageRunner*		fAutoCheckRunner;
			BMailSettings		fSettingsFile;

			int32				fNewMessages;
			bool				fCentralBeep;
				// TRUE to do a beep when the status window closes. This happens
				// when all mail has been received, so you get one beep for
				// everything rather than individual beeps for each mail
				// account.
				// Set to TRUE by the 'mcbp' message that the mail Notification
				// filter sends us, cleared when the beep is done.
			BList				fFetchDoneRespondents;
			BList				fQueries;

			LEDAnimation*		fLEDAnimation;

			BString				fAlertString;
};


MailDaemonApp::MailDaemonApp()
	:
	BApplication("application/x-vnd.Be-POST")
{
	sStatus = new BMailStatusWindow(BRect(40, 400, 360, 400), "Mail Status",
		fSettingsFile.ShowStatusWindow());
	fAutoCheckRunner = NULL;
}


MailDaemonApp::~MailDaemonApp()
{
	delete fAutoCheckRunner;

	for (int32 i = 0; i < fQueries.CountItems(); i++)
		delete (BQuery*)fQueries.ItemAt(i);

	delete fLEDAnimation;
}


void
MailDaemonApp::ReadyToRun()
{
	InstallDeskbarIcon();

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

	BString string;
	MDR_DIALECT_CHOICE(
		if (fNewMessages > 0)
			string << fNewMessages;
		else
			string << "No";
		if (fNewMessages != 1)
			string << " new messages.";
		else
			string << " new message.";,
		if (fNewMessages > 0)
			string << fNewMessages << " 通の未読メッセージがあります ";
		else
			string << "未読メッセージはありません";
	);
	fCentralBeep = false;
	sStatus->SetDefaultMessage(string);

	fLEDAnimation = new LEDAnimation;
	SetPulseRate(1000000);
}


void
MailDaemonApp::RefsReceived(BMessage* message)
{
	sStatus->Activate(true);

	entry_ref ref;
	for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
		BNode node(&ref);
		if (node.InitCheck() < B_OK)
			continue;

		BString uid;
		if (node.ReadAttrString("MAIL:unique_id", &uid) < 0)
			continue;

		int32 id;
		if (node.ReadAttr("MAIL:chain", B_INT32_TYPE, 0, &id, sizeof(id)) < 0)
			continue;

		int32 size;
		if (node.ReadAttr("MAIL:fullsize", B_SIZE_T_TYPE, 0, &size,
				sizeof(size)) < 0) {
			size = -1;
		}

		BPath path(&ref);
		BMailChainRunner* runner = GetMailChainRunner(id, sStatus);
		if (runner != NULL)
			runner->GetSingleMessage(uid.String(), size, &path);
	}
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


void
MailDaemonApp::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case 'moto':
			if (fSettingsFile.CheckOnlyIfPPPUp()) {
				// TODO: check whether internet is up and running!
			}
			// supposed to fall through
		case 'mbth':	// check & send messages
			msg->what = 'msnd';
			PostMessage(msg);
			// supposed to fall trough
		case 'mnow':	// check messages
			GetNewMessages(msg);
			break;

		case 'msnd':	// send messages
			SendPendingMessages(msg);
			break;

		case 'mrrs':
			fSettingsFile.Reload();
			_UpdateAutoCheck(fSettingsFile.AutoCheckInterval());
			sStatus->SetShowCriterion(fSettingsFile.ShowStatusWindow());
			break;
		case 'shst':	// when to show the status window
		{
			int32 mode;
			if (msg->FindInt32("ShowStatusWindow", &mode) == B_OK)
				sStatus->SetShowCriterion(mode);
			break;
		}

		case 'lkch':	// status window look changed
		case 'wsch':	// workspace changed
			sStatus->PostMessage(msg);
			break;

		case 'stwg':	// Status window gone
		{
			BMessage reply('mnuc');
			reply.AddInt32("num_new_messages", fNewMessages);

			while ((msg = (BMessage*)fFetchDoneRespondents.RemoveItem(0L))) {
				msg->SendReply(&reply);
				delete msg;
			}

			if (fAlertString != B_EMPTY_STRING) {
				fAlertString.Truncate(fAlertString.Length() - 1);
				BAlert* alert = new BAlert(MDR_DIALECT_CHOICE("New Messages",
					"新着メッセージ"), fAlertString.String(), "OK", NULL, NULL,
					B_WIDTH_AS_USUAL);
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

		case 'mnum':	// Number of new messages
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
			MDR_DIALECT_CHOICE(
				fAlertString << numMessages << " new message";
				if (numMessages > 1)
					fAlertString << 's';

				fAlertString << " for " << msg->FindString("chain_name")
					<< '\n';,

				fAlertString << msg->FindString("chain_name") << "より\n"
					<< numMessages << " 通のメッセージが届きました　　";
			);
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

			BString string;

			MDR_DIALECT_CHOICE(
				if (fNewMessages > 0)
					string << fNewMessages;
				else
					string << "No";
				if (fNewMessages != 1)
					string << " new messages.";
				else
					string << " new message.";,

				if (fNewMessages > 0)
					string << fNewMessages << " 通の未読メッセージがあります";
				else
					string << "未読メッセージはありません";
			);

			sStatus->SetDefaultMessage(string.String());
			break;
		}

		default:
			BApplication::MessageReceived(msg);
			break;
	}
}


void
MailDaemonApp::InstallDeskbarIcon()
{
	BDeskbar deskbar;

	if (!deskbar.HasItem("mail_daemon")) {
		BRoster roster;
		entry_ref ref;

		status_t status = roster.FindApp("application/x-vnd.Be-POST", &ref);
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


bool
MailDaemonApp::QuitRequested()
{
	RemoveDeskbarIcon();

	return true;
}


void
MailDaemonApp::RunChains(BList& list, BMessage* msg)
{
	BMailChain* chain;

	int32 index = 0, id;
	for (; msg->FindInt32("chain", index, &id) == B_OK; index++) {
		for (int32 i = 0; i < list.CountItems(); i++) {
			chain = (BMailChain*)list.ItemAt(i);

			if (chain->ID() == (unsigned)id) {
				chain->RunChain(sStatus, true, false, true);
				list.RemoveItem(i);	// the chain runner deletes the chain
				break;
			}
		}
	}

	if (index == 0) {
		// invoke all chains
		for (int32 i = 0; i < list.CountItems(); i++) {
			chain = (BMailChain*)list.ItemAt(i);

			chain->RunChain(sStatus, true, false, true);
		}
	} else {
		// delete unused chains
		for (int32 i = list.CountItems(); i-- > 0;)
			delete (BMailChain*)list.RemoveItem(i);
	}
}


void
MailDaemonApp::GetNewMessages(BMessage* msg)
{
	BList list;
	GetInboundMailChains(&list);

	RunChains(list, msg);
}


void
MailDaemonApp::SendPendingMessages(BMessage* msg)
{
	BVolumeRoster roster;
	BVolume volume;

	while (roster.GetNextVolume(&volume) == B_OK) {
		BQuery query;
		query.SetVolume(&volume);
		query.PushAttr(B_MAIL_ATTR_FLAGS);
		query.PushInt32(B_MAIL_PENDING);
		query.PushOp(B_EQ);

		query.PushAttr(B_MAIL_ATTR_FLAGS);
		query.PushInt32(B_MAIL_PENDING | B_MAIL_SAVE);
		query.PushOp(B_EQ);

		query.PushOp(B_OR);

		int32 chainID = -1;

		if (msg->FindInt32("chain", &chainID) == B_OK) {
			query.PushAttr("MAIL:chain");
			query.PushInt32(chainID);
			query.PushOp(B_EQ);
			query.PushOp(B_AND);
		} else
			chainID = -1;

		if (!msg->HasString("message_path")) {
			if (chainID == -1) {
				map<int32, snuzzwut*> messages;

				query.Fetch();
				BEntry entry;
				BPath path;
				BNode node;
				int32 chain;
				int32 defaultChain(BMailSettings().DefaultOutboundChainID());
				off_t size;

				while (query.GetNextEntry(&entry) == B_OK) {
					if (_IsEntryInTrash(entry))
						continue;

					while (node.SetTo(&entry) == B_BUSY)
						snooze(1000);
					if (!_IsPending(node))
						continue;

					if (node.ReadAttr("MAIL:chain", B_INT32_TYPE, 0, &chain, 4)
							< B_OK)
						chain = defaultChain;
					entry.GetPath(&path);
					node.GetSize(&size);
					if (messages[chain] == NULL) {
						messages[chain] = new snuzzwut;
						messages[chain]->bytes = 0;
					}

					messages[chain]->msgs += path.Path();
					messages[chain]->bytes += size;
				}

				map<int32, snuzzwut*>::iterator iter = messages.begin();
				map<int32, snuzzwut*>::iterator end = messages.end();
				while (iter != end) {
					if (iter->first > 0 && BMailChain(iter->first)
							.ChainDirection() == outbound) {
						BMailChainRunner* runner
							= GetMailChainRunner(iter->first, sStatus);
						runner->GetMessages(&messages[iter->first]->msgs,
							messages[iter->first]->bytes);
						delete messages[iter->first];
						runner->Stop();
					}

					iter++;
				}
			} else {
				BStringList ids;
				size_t bytes = 0;

				query.Fetch();
				BEntry entry;
				BPath path;
				BNode node;
				off_t size;

				while (query.GetNextEntry(&entry) == B_OK) {
					if (_IsEntryInTrash(entry))
						continue;

					node.SetTo(&entry);
					if (!_IsPending(node))
						continue;

					entry.GetPath(&path);
					node.GetSize(&size);
					ids += path.Path();
					bytes += size;
				}

				BMailChainRunner* runner
					= GetMailChainRunner(chainID, sStatus);
				runner->GetMessages(&ids, bytes);
				runner->Stop();
			}
		} else {
			const char* path;
			if (msg->FindString("message_path", &path) != B_OK)
				return;

			off_t size;
			if (BNode(path).GetSize(&size) != B_OK)
				return;

			BStringList ids;
			ids += path;

			BMailChainRunner* runner = GetMailChainRunner(chainID, sStatus);
			runner->GetMessages(&ids, size);
			runner->Stop();
		}
	}
}


void
MailDaemonApp::Pulse()
{
	bigtime_t idle = idle_time();
	if (fLEDAnimation->IsRunning() && idle < 100000)
		fLEDAnimation->Stop();
}


/*!	Work-around for a broken index that contains out-of-date information.
*/

/* static */
bool
MailDaemonApp::_IsPending(BNode& node)
{
	int32 flags;
	if (node.ReadAttr(B_MAIL_ATTR_FLAGS, B_INT32_TYPE, 0, &flags, sizeof(int32))
			!= (ssize_t)sizeof(int32))
		return false;

	return (flags & B_MAIL_PENDING) != 0;
}


/* static */
bool
MailDaemonApp::_IsEntryInTrash(BEntry& entry)
{
	entry_ref ref;

	entry.GetRef(&ref);
	BPath trashPath;
	BPath entryPath(&entry);
	BVolume volume(ref.device);
	if (volume.InitCheck() == B_OK) {
		find_directory(B_TRASH_DIRECTORY, &trashPath, false, &volume);
		if (strncmp(entryPath.Path(), trashPath.Path(),
			strlen(trashPath.Path())) == 0)
			return true;
	}

	return false;
}


//	#pragma mark -


void
makeIndices()
{
	const char* stringIndices[] = {
		B_MAIL_ATTR_ACCOUNT, B_MAIL_ATTR_CC, B_MAIL_ATTR_FROM, B_MAIL_ATTR_NAME,
		B_MAIL_ATTR_PRIORITY, B_MAIL_ATTR_REPLY, B_MAIL_ATTR_STATUS,
		B_MAIL_ATTR_SUBJECT, B_MAIL_ATTR_TO, B_MAIL_ATTR_THREAD,
		NULL
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
		fs_create_index(device, "MAIL:chain", B_INT32_TYPE, 0);
		fs_create_index(device, "MAIL:pending_chain", B_INT32_TYPE, 0);
	}
}


void
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


void
makeMimeType(bool remakeMIMETypes)
{
	// Add MIME database entries for the e-mail file types we handle.  Either
	// do a full rebuild from nothing, or just add on the new attributes that
	// we support which the regular BeOS mail daemon didn't have.

	const char* types[2] = {"text/x-email", "text/x-partial-email"};
	BMimeType mime;
	BMessage info;

	for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
		info.MakeEmpty();
		mime.SetTo(types[i]);
		if (mime.InitCheck() != B_OK) {
			fputs("could not init mime type.\n", stderr);
			return;
		}

		if (!mime.IsInstalled() || remakeMIMETypes) {
			// install the full mime type
			mime.Delete ();
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
			mime.SetAttrInfo(&info);

			if (i == 0) {
				mime.SetShortDescription("E-mail");
				mime.SetLongDescription("Electronic Mail Message");
				mime.SetPreferredApp("application/x-vnd.Be-MAIL");
			} else {
				mime.SetShortDescription("Partial E-mail");
				mime.SetLongDescription("A Partially Downloaded E-mail");
				mime.SetPreferredApp("application/x-vnd.Be-POST");
			}
		} else {
			// Just add the e-mail related attribute types we use to the MIME
			// system.
			mime.GetAttrInfo(&info);
			bool hasAccount = false;
			bool hasThread = false;
			bool hasSize = false;
			const char* result;
			for (int32 index = 0; info.FindString("attr:name", index, &result)
					== B_OK; index++) {
				if (!strcmp(result, B_MAIL_ATTR_ACCOUNT))
					hasAccount = true;
				if (!strcmp(result, B_MAIL_ATTR_THREAD))
					hasThread = true;
				if (!strcmp(result, "MAIL:fullsize"))
					hasSize = true;
			}

			if (!hasAccount) {
				addAttribute(info, B_MAIL_ATTR_ACCOUNT, "Account",
					B_STRING_TYPE, true, false, 100);
			}
			if (!hasThread)
				addAttribute(info, B_MAIL_ATTR_THREAD, "Thread");
			/*if (!hasSize)
				addAttribute(info,"MAIL:fullsize","Message Size",B_SIZE_T_TYPE,true,false,100);*/
			// TODO: Tracker can't display SIZT attributes. What a pain.
			if (!hasAccount || !hasThread/* || !hasSize*/)
				mime.SetAttrInfo(&info);
		}
		mime.Unset();
	}
}


int
main(int argc, const char** argv)
{
	bool remakeMIMETypes = false;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-E") == 0) {
			if (!BMailSettings().DaemonAutoStarts())
				return 0;
		}
		if (strcmp(argv[i], "-M") == 0) {
			remakeMIMETypes = true;
		}
	}

	MailDaemonApp app;

	// install MimeTypes, attributes, indices, and the
	// system beep add startup

	makeMimeType(remakeMIMETypes);
	makeIndices();
	add_system_beep_event("New E-mail");

	be_app->Run();
	return 0;
}

