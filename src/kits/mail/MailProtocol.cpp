/* BMailProtocol - the base class for protocol filters
**
** Copyright 2001-2003 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <stdio.h>
#include <fs_attr.h>
#include <stdlib.h>
#include <assert.h>

#include <Alert.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Query.h>
#include <E-mail.h>
#include <Node.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>
#include <StringList.h>
#include <VolumeRoster.h>

#include <mail_util.h>
#include <MailAddon.h>
#include <MailDaemon.h>
#include <MailProtocol.h>
#include <MailSettings.h>

#include "HaikuMailFormatFilter.h"


using std::map;


MailFilter::MailFilter(MailProtocol& protocol, AddonSettings* settings)
	:
	fMailProtocol(protocol),
	fAddonSettings(settings)
{

}


MailFilter::~MailFilter()
{
	
}


void
MailFilter::HeaderFetched(const entry_ref& ref, BFile* file)
{

}


void
MailFilter::BodyFetched(const entry_ref& ref, BFile* file)
{

}


void
MailFilter::MailboxSynced(status_t status)
{

}


void
MailFilter::MessageReadyToSend(const entry_ref& ref, BFile* file)
{

}


void
MailFilter::MessageSent(const entry_ref& ref, BFile* file)
{

}


MailProtocol::MailProtocol(BMailAccountSettings* settings)
	:
	fMailNotifier(NULL),
	fProtocolThread(NULL)
{
	fAccountSettings = *settings;

	AddFilter(new HaikuMailFormatFilter(*this, settings));
}


MailProtocol::~MailProtocol()
{
	delete fMailNotifier;

	for (int i = 0; i < fFilterList.CountItems(); i++)
		delete fFilterList.ItemAt(i);

	map<entry_ref, image_id>::iterator it = fFilterImages.begin();
	for (; it != fFilterImages.end(); it++)
		unload_add_on(it->second);
}


BMailAccountSettings&
MailProtocol::AccountSettings()
{
	return fAccountSettings;
}


void
MailProtocol::SetProtocolThread(MailProtocolThread* protocolThread)
{
	if (fProtocolThread) {
		fProtocolThread->Lock();
		for (int i = 0; i < fHandlerList.CountItems(); i++)
			fProtocolThread->RemoveHandler(fHandlerList.ItemAt(i));
		fProtocolThread->Unlock();
	}

	fProtocolThread = protocolThread;

	if (!fProtocolThread)
		return;

	fProtocolThread->Lock();
	for (int i = 0; i < fHandlerList.CountItems(); i++)
		fProtocolThread->AddHandler(fHandlerList.ItemAt(i));
	fProtocolThread->Unlock();

	AddedToLooper();
}


MailProtocolThread*
MailProtocol::Looper()
{
	return fProtocolThread;
}


bool
MailProtocol::AddHandler(BHandler* handler)
{
	if (!fHandlerList.AddItem(handler))
		return false;
	if (fProtocolThread) {
		fProtocolThread->Lock();
		fProtocolThread->AddHandler(handler);
		fProtocolThread->Unlock();
	}
	return true;
}


bool
MailProtocol::RemoveHandler(BHandler* handler)
{
	if (!fHandlerList.RemoveItem(handler))
		return false;
	if (fProtocolThread) {
		fProtocolThread->Lock();
		fProtocolThread->RemoveHandler(handler);
		fProtocolThread->Unlock();
	}
	return true;
}


void
MailProtocol::SetMailNotifier(MailNotifier* mailNotifier)
{
	delete fMailNotifier;
	fMailNotifier = mailNotifier;
}


void
MailProtocol::ShowError(const char* error)
{
	if (fMailNotifier)
		fMailNotifier->ShowError(error);
}


void
MailProtocol::ShowMessage(const char* message)
{
	if (fMailNotifier)
		fMailNotifier->ShowMessage(message);
}


void
MailProtocol::SetTotalItems(int32 items)
{
	if (fMailNotifier)
		fMailNotifier->SetTotalItems(items);
}


void
MailProtocol::SetTotalItemsSize(int32 size)
{
	if (fMailNotifier)
		fMailNotifier->SetTotalItemsSize(size);
}


void
MailProtocol::ReportProgress(int bytes, int messages, const char* message)
{
	if (fMailNotifier)
		fMailNotifier->ReportProgress(bytes, messages, message);
}


void
MailProtocol::ResetProgress(const char* message)
{
	if (fMailNotifier)
		fMailNotifier->ResetProgress(message);
}


bool
MailProtocol::AddFilter(MailFilter* filter)
{
	return fFilterList.AddItem(filter);
}


int32
MailProtocol::CountFilter()
{
	return fFilterList.CountItems();
}


MailFilter*
MailProtocol::FilterAt(int32 index)
{
	return fFilterList.ItemAt(index);
}


MailFilter*
MailProtocol::RemoveFilter(int32 index)
{
	return fFilterList.RemoveItemAt(index);
}


bool
MailProtocol::RemoveFilter(MailFilter* filter)
{
	return fFilterList.RemoveItem(filter);
}


void
MailProtocol::NotifyNewMessagesToFetch(int32 nMessages)
{
	ResetProgress();
	SetTotalItems(nMessages);
}


void
MailProtocol::NotifyHeaderFetched(const entry_ref& ref, BFile* data)
{
	for (int i = 0; i < fFilterList.CountItems(); i++)
		fFilterList.ItemAt(i)->HeaderFetched(ref, data);
}


void
MailProtocol::NotifyBodyFetched(const entry_ref& ref, BFile* data)
{
	for (int i = 0; i < fFilterList.CountItems(); i++)
		fFilterList.ItemAt(i)->BodyFetched(ref, data);
}


void
MailProtocol::NotifyMessageReadyToSend(const entry_ref& ref, BFile* data)
{
	for (int i = 0; i < fFilterList.CountItems(); i++)
		fFilterList.ItemAt(i)->MessageReadyToSend(ref, data);
}


void
MailProtocol::NotifyMessageSent(const entry_ref& ref, BFile* data)
{
	for (int i = 0; i < fFilterList.CountItems(); i++)
		fFilterList.ItemAt(i)->MessageSent(ref, data);
}


status_t
MailProtocol::MoveMessage(const entry_ref& ref, BDirectory& dir)
{
	BEntry entry(&ref);
	return entry.MoveTo(&dir);
}


status_t
MailProtocol::DeleteMessage(const entry_ref& ref)
{
	BEntry entry(&ref);
	return entry.Remove();
}


void
MailProtocol::FileRenamed(const entry_ref& from, const entry_ref& to)
{

}


void
MailProtocol::FileDeleted(const node_ref& node)
{

}


void
MailProtocol::LoadFilters(MailAddonSettings& settings)
{
	for (int i = 0; i < settings.CountFilterSettings(); i++) {
		AddonSettings* filterSettings = settings.FilterSettingsAt(i);
		MailFilter* filter = _LoadFilter(filterSettings);
		if (!filter)
			continue;
		AddFilter(filter);
	}
}


MailFilter*
MailProtocol::_LoadFilter(AddonSettings* filterSettings)
{
	const entry_ref& ref = filterSettings->AddonRef();
	map<entry_ref, image_id>::iterator it = fFilterImages.find(ref);
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

	MailFilter* (*instantiate_mailfilter)(MailProtocol& protocol,
		AddonSettings* settings);
	if (get_image_symbol(image, "instantiate_mailfilter",
		B_SYMBOL_TYPE_TEXT, (void **)&instantiate_mailfilter)
			!= B_OK) {
		unload_add_on(image);
		return NULL;
	}

	fFilterImages[ref] = image;
	return (*instantiate_mailfilter)(*this, filterSettings);
}


InboundProtocol::InboundProtocol(BMailAccountSettings* settings)
	:
	MailProtocol(settings)
{
	LoadFilters(fAccountSettings.InboundSettings());
}


InboundProtocol::~InboundProtocol()
{
	
}


status_t
InboundProtocol::AppendMessage(const entry_ref& ref)
{
	return false;
}


status_t
InboundProtocol::MarkMessageAsRead(const entry_ref& ref, read_flags flag)
{
	BNode node(&ref);
	return write_read_attr(node, flag);
}


OutboundProtocol::OutboundProtocol(BMailAccountSettings* settings)
	:
	MailProtocol(settings)
{
	LoadFilters(fAccountSettings.OutboundSettings());
}


OutboundProtocol::~OutboundProtocol()
{
	
}


const uint32 kMsgMoveFile = '&MoF';
const uint32 kMsgDeleteFile = '&DeF';
const uint32 kMsgFileRenamed = '&FiR';
const uint32 kMsgFileDeleted = '&FDe';
const uint32 kMsgInit = '&Ini';


MailProtocolThread::MailProtocolThread(MailProtocol* protocol)
	:
	fMailProtocol(protocol)
{
	PostMessage(kMsgInit);
}


void
MailProtocolThread::SetStopNow()
{
	fMailProtocol->SetStopNow();
}


void
MailProtocolThread::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case kMsgInit:
		fMailProtocol->SetProtocolThread(this);
		break;

	case kMsgMoveFile:
	{
		entry_ref file;
		message->FindRef("file", &file);
		entry_ref dir;
		message->FindRef("directory", &dir);
		BDirectory directory(&dir);
		fMailProtocol->MoveMessage(file, directory);
		break;
	}

	case kMsgDeleteFile:
	{
		entry_ref file;
		message->FindRef("file", &file);
		fMailProtocol->DeleteMessage(file);
		break;
	}

	case kMsgFileRenamed:
	{
		entry_ref from;
		message->FindRef("from", &from);
		entry_ref to;
		message->FindRef("to", &to);
		fMailProtocol->FileRenamed(from, to);
	}

	case kMsgFileDeleted:
	{
		node_ref node;
		message->FindInt32("device",&node.device);
		message->FindInt64("node", &node.node);
		fMailProtocol->FileDeleted(node);
	}

	default:
		BLooper::MessageReceived(message);
	}
}


void
MailProtocolThread::TriggerFileMove(const entry_ref& ref, BDirectory& dir)
{
	BMessage message(kMsgMoveFile);
	message.AddRef("file", &ref);
	BEntry entry;
	dir.GetEntry(&entry);
	entry_ref dirRef;
	entry.GetRef(&dirRef);
	message.AddRef("directory", &dirRef);
	PostMessage(&message);
}


void
MailProtocolThread::TriggerFileDeletion(const entry_ref& ref)
{
	BMessage message(kMsgDeleteFile);
	message.AddRef("file", &ref);
	PostMessage(&message);
}


void
MailProtocolThread::TriggerFileRenamed(const entry_ref& from,
	const entry_ref& to)
{
	BMessage message(kMsgFileRenamed);
	message.AddRef("from", &from);
	message.AddRef("to", &to);
	PostMessage(&message);
}


void
MailProtocolThread::TriggerFileDeleted(const node_ref& node)
{
	BMessage message(kMsgFileDeleted);
	message.AddInt32("device", node.device);
	message.AddInt64("node", node.node);
	PostMessage(&message);
}


const uint32 kMsgSyncMessages = '&SyM';
const uint32 kMsgDeleteMessage = '&DeM';
const uint32 kMsgAppendMessage = '&ApM';


InboundProtocolThread::InboundProtocolThread(InboundProtocol* protocol)
	:
	MailProtocolThread(protocol),
	fProtocol(protocol)
{

}


InboundProtocolThread::~InboundProtocolThread()
{
	fProtocol->SetProtocolThread(NULL);
}


void
InboundProtocolThread::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case kMsgSyncMessages:
	{
		status_t status = fProtocol->SyncMessages();
		_NotiyMailboxSynced(status);
		break;
	}

	case kMsgFetchBody:
	{
		entry_ref ref;
		message->FindRef("ref", &ref);
		status_t status = fProtocol->FetchBody(ref);

		BMessenger target;
		if (message->FindMessenger("target", &target) != B_OK)
			break;

		BMessage message(kMsgBodyFetched);
		message.AddInt32("status", status);
		message.AddRef("ref", &ref);
		target.SendMessage(&message);
		break;
	}

	case kMsgMarkMessageAsRead:
	{
		entry_ref ref;
		message->FindRef("ref", &ref);
		read_flags read = (read_flags)message->FindInt32("read");
		fProtocol->MarkMessageAsRead(ref, read);
		break;
	}

	case kMsgDeleteMessage:
	{
		entry_ref ref;
		message->FindRef("ref", &ref);
		fProtocol->DeleteMessage(ref);
		break;
	}

	case kMsgAppendMessage:
	{
		entry_ref ref;
		message->FindRef("ref", &ref);
		fProtocol->AppendMessage(ref);
		break;
	}

	default:
		MailProtocolThread::MessageReceived(message);
	}
}


void
InboundProtocolThread::SyncMessages()
{
	PostMessage(kMsgSyncMessages);
}


void
InboundProtocolThread::FetchBody(const entry_ref& ref, BMessenger* listener)
{
	BMessage message(kMsgFetchBody);
	message.AddRef("ref", &ref);
	if (listener)
		message.AddMessenger("target", *listener);
	PostMessage(&message);
}


void
InboundProtocolThread::MarkMessageAsRead(const entry_ref& ref, read_flags flag)
{
	BMessage message(kMsgMarkMessageAsRead);
	message.AddRef("ref", &ref);
	message.AddInt32("read", flag);
	PostMessage(&message);
}


void
InboundProtocolThread::DeleteMessage(const entry_ref& ref)
{
	BMessage message(kMsgDeleteMessage);
	message.AddRef("ref", &ref);
	PostMessage(&message);
}


void
InboundProtocolThread::AppendMessage(const entry_ref& ref)
{
	BMessage message(kMsgAppendMessage);
	message.AddRef("ref", &ref);
	PostMessage(&message);
}


void
InboundProtocolThread::_NotiyMailboxSynced(status_t status)
{
	for (int i = 0; i < fProtocol->CountFilter(); i++)
		fProtocol->FilterAt(i)->MailboxSynced(status);
}


const uint32 kMsgSendMessage = '&SeM';


OutboundProtocolThread::OutboundProtocolThread(OutboundProtocol* protocol)
	:
	MailProtocolThread(protocol),
	fProtocol(protocol)
{

}


OutboundProtocolThread::~OutboundProtocolThread()
{
	fProtocol->SetProtocolThread(NULL);
}


void
OutboundProtocolThread::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case kMsgSendMessage:
	{
		std::vector<entry_ref> mails;
		for (int32 i = 0; ;i++) {
			entry_ref ref;
			if (message->FindRef("ref", i, &ref) != B_OK)
				break;
			mails.push_back(ref);
		}
		size_t size = message->FindInt32("size");
		fProtocol->SendMessages(mails, size);
		break;
	}

	default:
		MailProtocolThread::MessageReceived(message);
	}
}


void
OutboundProtocolThread::SendMessages(const std::vector<entry_ref>& mails,
	size_t totalBytes)
{
	BMessage message(kMsgSendMessage);
	for (unsigned int i = 0; i < mails.size(); i++)
		message.AddRef("ref", &mails[i]);
	message.AddInt32("size", totalBytes);
	PostMessage(&message);
}
