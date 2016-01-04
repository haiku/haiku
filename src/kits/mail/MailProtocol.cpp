/*
 * Copyright 2011-2016, Haiku, Inc. All rights reserved.
 * Copyright 2001-2003 Dr. Zoidberg Enterprises. All rights reserved.
 */


#include <stdio.h>
#include <stdlib.h>

#include <fs_attr.h>

#include <Alert.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <E-mail.h>
#include <Locker.h>
#include <Node.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Query.h>
#include <Roster.h>
#include <String.h>
#include <StringList.h>
#include <VolumeRoster.h>

#include <MailFilter.h>
#include <MailDaemon.h>
#include <MailProtocol.h>
#include <MailSettings.h>

#include <mail_util.h>
#include <MailPrivate.h>
#include <NodeMessage.h>

#include "HaikuMailFormatFilter.h"


using namespace BPrivate;


const uint32 kMsgDeleteMessage = '&DeM';
const uint32 kMsgAppendMessage = '&ApM';


BMailProtocol::BMailProtocol(const char* name,
	const BMailAccountSettings& settings)
	:
	BLooper(_LooperName(name, settings)),
	fAccountSettings(settings),
	fMailNotifier(NULL)
{
	AddFilter(new HaikuMailFormatFilter(*this, settings));
}


BMailProtocol::~BMailProtocol()
{
	delete fMailNotifier;

	for (int i = 0; i < fFilterList.CountItems(); i++)
		delete fFilterList.ItemAt(i);

	std::map<entry_ref, image_id>::iterator it = fFilterImages.begin();
	for (; it != fFilterImages.end(); it++)
		unload_add_on(it->second);
}


const BMailAccountSettings&
BMailProtocol::AccountSettings() const
{
	return fAccountSettings;
}


void
BMailProtocol::SetMailNotifier(BMailNotifier* mailNotifier)
{
	delete fMailNotifier;
	fMailNotifier = mailNotifier;
}


BMailNotifier*
BMailProtocol::MailNotifier() const
{
	return fMailNotifier;
}


bool
BMailProtocol::AddFilter(BMailFilter* filter)
{
	BLocker locker(this);
	return fFilterList.AddItem(filter);
}


int32
BMailProtocol::CountFilter() const
{
	BLocker locker(this);
	return fFilterList.CountItems();
}


BMailFilter*
BMailProtocol::FilterAt(int32 index) const
{
	BLocker locker(this);
	return fFilterList.ItemAt(index);
}


BMailFilter*
BMailProtocol::RemoveFilter(int32 index)
{
	BLocker locker(this);
	return fFilterList.RemoveItemAt(index);
}


bool
BMailProtocol::RemoveFilter(BMailFilter* filter)
{
	BLocker locker(this);
	return fFilterList.RemoveItem(filter);
}


void
BMailProtocol::MessageReceived(BMessage* message)
{
	BLooper::MessageReceived(message);
}


status_t
BMailProtocol::MoveMessage(const entry_ref& ref, BDirectory& dir)
{
	BEntry entry(&ref);
	return entry.MoveTo(&dir);
}


status_t
BMailProtocol::DeleteMessage(const entry_ref& ref)
{
	BEntry entry(&ref);
	return entry.Remove();
}


void
BMailProtocol::ShowError(const char* error)
{
	if (MailNotifier() != NULL)
		MailNotifier()->ShowError(error);
}


void
BMailProtocol::ShowMessage(const char* message)
{
	if (MailNotifier() != NULL)
		MailNotifier()->ShowMessage(message);
}


void
BMailProtocol::SetTotalItems(uint32 items)
{
	if (MailNotifier() != NULL)
		MailNotifier()->SetTotalItems(items);
}


void
BMailProtocol::SetTotalItemsSize(uint64 size)
{
	if (MailNotifier() != NULL)
		MailNotifier()->SetTotalItemsSize(size);
}


void
BMailProtocol::ReportProgress(uint32 messages, uint64 bytes,
	const char* message)
{
	if (MailNotifier() != NULL)
		MailNotifier()->ReportProgress(messages, bytes, message);
}


void
BMailProtocol::ResetProgress(const char* message)
{
	if (MailNotifier() != NULL)
		MailNotifier()->ResetProgress(message);
}


void
BMailProtocol::NotifyNewMessagesToFetch(int32 count)
{
	ResetProgress();
	SetTotalItems(count);
}


BMailFilterAction
BMailProtocol::ProcessHeaderFetched(entry_ref& ref, BFile& file,
	BMessage& attributes)
{
	BMailFilterAction action = _ProcessHeaderFetched(ref, file, attributes);
	if (action >= B_OK && action != B_DELETE_MAIL_ACTION)
		file << attributes;

	return action;
}


void
BMailProtocol::NotifyBodyFetched(const entry_ref& ref, BFile& file,
	BMessage& attributes)
{
	_NotifyBodyFetched(ref, file, attributes);
	file << attributes;
}


BMailFilterAction
BMailProtocol::ProcessMessageFetched(entry_ref& ref, BFile& file,
	BMessage& attributes)
{
	BMailFilterAction action = _ProcessHeaderFetched(ref, file, attributes);
	if (action >= B_OK && action != B_DELETE_MAIL_ACTION) {
		_NotifyBodyFetched(ref, file, attributes);
		file << attributes;
	}

	return action;
}


void
BMailProtocol::NotifyMessageReadyToSend(const entry_ref& ref, BFile& file)
{
	for (int i = 0; i < fFilterList.CountItems(); i++)
		fFilterList.ItemAt(i)->MessageReadyToSend(ref, file);
}


void
BMailProtocol::NotifyMessageSent(const entry_ref& ref, BFile& file)
{
	for (int i = 0; i < fFilterList.CountItems(); i++)
		fFilterList.ItemAt(i)->MessageSent(ref, file);
}


void
BMailProtocol::LoadFilters(const BMailProtocolSettings& settings)
{
	for (int i = 0; i < settings.CountFilterSettings(); i++) {
		BMailAddOnSettings* filterSettings = settings.FilterSettingsAt(i);
		BMailFilter* filter = _LoadFilter(*filterSettings);
		if (filter != NULL)
			AddFilter(filter);
	}
}


/*static*/ BString
BMailProtocol::_LooperName(const char* addOnName,
	const BMailAccountSettings& settings)
{
	BString name = addOnName;

	const char* accountName = settings.Name();
	if (accountName != NULL && accountName[0] != '\0')
		name << " " << accountName;

	return name;
}


BMailFilter*
BMailProtocol::_LoadFilter(const BMailAddOnSettings& settings)
{
	const entry_ref& ref = settings.AddOnRef();
	std::map<entry_ref, image_id>::iterator it = fFilterImages.find(ref);
	image_id image;
	if (it != fFilterImages.end())
		image = it->second;
	else {
		BEntry entry(&ref);
		BPath path(&entry);
		image = load_add_on(path.Path());
	}
	if (image < 0)
		return NULL;

	BMailFilter* (*instantiateFilter)(BMailProtocol& protocol,
		const BMailAddOnSettings& settings);
	if (get_image_symbol(image, "instantiate_filter", B_SYMBOL_TYPE_TEXT,
			(void**)&instantiateFilter) != B_OK) {
		unload_add_on(image);
		return NULL;
	}

	fFilterImages[ref] = image;
	return instantiateFilter(*this, settings);
}


BMailFilterAction
BMailProtocol::_ProcessHeaderFetched(entry_ref& ref, BFile& file,
	BMessage& attributes)
{
	entry_ref outRef = ref;

	for (int i = 0; i < fFilterList.CountItems(); i++) {
		BMailFilterAction action = fFilterList.ItemAt(i)->HeaderFetched(outRef,
			file, attributes);
		if (action == B_DELETE_MAIL_ACTION) {
			// We have to delete the message
			BEntry entry(&ref);
			status_t status = entry.Remove();
			if (status != B_OK) {
				fprintf(stderr, "BMailProtocol::NotifyHeaderFetched(): could "
					"not delete mail: %s\n", strerror(status));
			}
			return B_DELETE_MAIL_ACTION;
		}
	}

	if (ref == outRef)
		return B_NO_MAIL_ACTION;

	// We have to rename the file
	node_ref newParentRef;
	newParentRef.device = outRef.device;
	newParentRef.node = outRef.directory;

	BDirectory newParent(&newParentRef);
	status_t status = newParent.InitCheck();
	BString workerName;
	if (status == B_OK) {
		int32 uniqueNumber = 1;
		do {
			workerName = outRef.name;
			if (uniqueNumber > 1)
				workerName << "_" << uniqueNumber;

			// TODO: support copying to another device!
			BEntry entry(&ref);
			status = entry.Rename(workerName);

			uniqueNumber++;
		} while (status == B_FILE_EXISTS);
	}

	if (status != B_OK) {
		fprintf(stderr, "BMailProtocol::NotifyHeaderFetched(): could not "
			"rename mail (%s)! (should be: %s)\n", strerror(status),
			workerName.String());
	}

	ref = outRef;
	ref.set_name(workerName.String());

	return B_MOVE_MAIL_ACTION;
}


void
BMailProtocol::_NotifyBodyFetched(const entry_ref& ref, BFile& file,
	BMessage& attributes)
{
	for (int i = 0; i < fFilterList.CountItems(); i++)
		fFilterList.ItemAt(i)->BodyFetched(ref, file, attributes);
}


// #pragma mark -


BInboundMailProtocol::BInboundMailProtocol(const char* name,
	const BMailAccountSettings& settings)
	:
	BMailProtocol(name, settings)
{
	LoadFilters(fAccountSettings.InboundSettings());
}


BInboundMailProtocol::~BInboundMailProtocol()
{
}


void
BInboundMailProtocol::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSyncMessages:
		{
			NotiyMailboxSynchronized(SyncMessages());
			break;
		}

		case kMsgFetchBody:
		{
			entry_ref ref;
			if (message->FindRef("ref", &ref) != B_OK)
				break;

			BMessenger target;
			message->FindMessenger("target", &target);

			status_t status = HandleFetchBody(ref, target);
			if (status != B_OK)
				ReplyBodyFetched(target, ref, status);
			break;
		}

		case kMsgMarkMessageAsRead:
		{
			entry_ref ref;
			message->FindRef("ref", &ref);
			read_flags read = (read_flags)message->FindInt32("read");
			MarkMessageAsRead(ref, read);
			break;
		}

		case kMsgDeleteMessage:
		{
			entry_ref ref;
			message->FindRef("ref", &ref);
			DeleteMessage(ref);
			break;
		}

		case kMsgAppendMessage:
		{
			entry_ref ref;
			message->FindRef("ref", &ref);
			AppendMessage(ref);
			break;
		}

		default:
			BMailProtocol::MessageReceived(message);
			break;
	}
}


status_t
BInboundMailProtocol::FetchBody(const entry_ref& ref, BMessenger* replyTo)
{
	BMessage message(kMsgFetchBody);
	message.AddRef("ref", &ref);
	if (replyTo != NULL)
		message.AddMessenger("target", *replyTo);

	return BMessenger(this).SendMessage(&message);
}


status_t
BInboundMailProtocol::MarkMessageAsRead(const entry_ref& ref, read_flags flag)
{
	BNode node(&ref);
	return write_read_attr(node, flag);
}


status_t
BInboundMailProtocol::DeleteMessage(const entry_ref& ref)
{
	return B_ERROR;
}


status_t
BInboundMailProtocol::AppendMessage(const entry_ref& ref)
{
	return B_OK;
}


/*static*/ void
BInboundMailProtocol::ReplyBodyFetched(const BMessenger& replyTo,
	const entry_ref& ref, status_t status)
{
	BMessage message(B_MAIL_BODY_FETCHED);
	message.AddInt32("status", status);
	message.AddRef("ref", &ref);
	replyTo.SendMessage(&message);
}


void
BInboundMailProtocol::NotiyMailboxSynchronized(status_t status)
{
	for (int32 i = 0; i < CountFilter(); i++)
		FilterAt(i)->MailboxSynchronized(status);
}


// #pragma mark -


BOutboundMailProtocol::BOutboundMailProtocol(const char* name,
	const BMailAccountSettings& settings)
	:
	BMailProtocol(name, settings)
{
	LoadFilters(fAccountSettings.OutboundSettings());
}


BOutboundMailProtocol::~BOutboundMailProtocol()
{
}


status_t
BOutboundMailProtocol::SendMessages(const BMessage& files, off_t totalBytes)
{
	BMessage message(kMsgSendMessages);
	message.Append(files);
	message.AddInt64("bytes", totalBytes);

	return BMessenger(this).SendMessage(&message);
}


void
BOutboundMailProtocol::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSendMessages:
			HandleSendMessages(*message, message->FindInt64("bytes"));
			break;

		default:
			BMailProtocol::MessageReceived(message);
	}
}
