/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "Commands.h"

#include <stdlib.h>

#include <AutoDeleter.h>


#define DEBUG_IMAP_HANDLER
#ifdef DEBUG_IMAP_HANDLER
#	include <stdio.h>
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) ;
#endif


using namespace BPrivate;


static void
PutFlag(BString& string, const char* flag)
{
	if (!string.IsEmpty())
		string += " ";
	string += flag;
}


static BString
GenerateFlagString(uint32 flags)
{
	BString string;

	if ((flags & IMAP::kSeen) != 0)
		PutFlag(string, "\\Seen ");
	if ((flags & IMAP::kAnswered) != 0)
		PutFlag(string, "\\Answered ");
	if ((flags & IMAP::kFlagged) != 0)
		PutFlag(string, "\\Flagged ");
	if ((flags & IMAP::kDeleted) != 0)
		PutFlag(string, "\\Deleted ");
	if ((flags & IMAP::kDraft) != 0)
		PutFlag(string, "\\Draft ");

	return string;
}


static uint32
ParseFlags(IMAP::ArgumentList& list)
{
	uint32 flags = 0;
	for (int32 i = 0; i < list.CountItems(); i++) {
		if (list.EqualsAt(i, "\\Seen"))
			flags |= IMAP::kSeen;
		else if (list.EqualsAt(i, "\\Answered"))
			flags |= IMAP::kAnswered;
		else if (list.EqualsAt(i, "\\Flagged"))
			flags |= IMAP::kFlagged;
		else if (list.EqualsAt(i, "\\Deleted"))
			flags |= IMAP::kDeleted;
		else if (list.EqualsAt(i, "\\Draft"))
			flags |= IMAP::kDraft;
	}
	return flags;
}


// #pragma mark -


namespace IMAP {


Handler::Handler()
{
}


Handler::~Handler()
{
}


// #pragma mark -


Command::~Command()
{
}


status_t
Command::HandleTagged(Response& response)
{
	if (response.StringAt(0) == "OK")
		return B_OK;
	if (response.StringAt(0) == "BAD")
		return B_BAD_VALUE;
	if (response.StringAt(0) == "NO")
		return B_NOT_ALLOWED;

	return B_ERROR;
}


// #pragma mark -

#if 0
HandlerListener::~HandlerListener()
{
}


void
HandlerListener::ExpungeReceived(int32 number)
{
}


void
HandlerListener::ExistsReceived(int32 number)
{
}
#endif

// #pragma mark -


RawCommand::RawCommand(const BString& command)
	:
	fCommand(command)
{
}


BString
RawCommand::CommandString()
{
	return fCommand;
}


// #pragma mark -


LoginCommand::LoginCommand(const char* user, const char* password)
	:
	fUser(user),
	fPassword(password)
{
}


BString
LoginCommand::CommandString()
{
	BString command = "LOGIN ";
	command << "\"" << fUser << "\" " << "\"" << fPassword << "\"";

	return command;
}


bool
LoginCommand::HandleUntagged(Response& response)
{
	if (!response.EqualsAt(0, "OK") || !response.IsListAt(1, '['))
		return false;

	// TODO: we only support capabilities at the moment
	ArgumentList& list = response.ListAt(1);
	if (!list.EqualsAt(0, "CAPABILITY"))
		return false;

	fCapabilities.MakeEmpty();
	while (list.CountItems() > 1)
		fCapabilities.AddItem(list.RemoveItemAt(1));

	TRACE("CAPABILITY: %s\n", fCapabilities.ToString().String());
	return true;
}


// #pragma mark -


SelectCommand::SelectCommand()
	:
	fNextUID(0),
	fUIDValidity(0)
{
}


SelectCommand::SelectCommand(const char* name)
	:
	fMailboxName(name),
	fNextUID(0),
	fUIDValidity(0)
{
}


BString
SelectCommand::CommandString()
{
	if (fMailboxName == "")
		return "";

	BString command = "SELECT \"";
	// TODO: properly quote the string!
	command += fMailboxName;
	command += "\"";
	return command;
}


bool
SelectCommand::HandleUntagged(Response& response)
{
	if (response.EqualsAt(0, "OK") && response.IsListAt(1, '[')) {
		const ArgumentList& arguments = response.ListAt(1);
		if (arguments.EqualsAt(0, "UIDVALIDITY")
			&& arguments.IsNumberAt(1)) {
			fUIDValidity = arguments.NumberAt(1);
			return true;
		} else if (arguments.EqualsAt(0, "UIDNEXT")
			&& arguments.IsNumberAt(1)) {
			fNextUID = arguments.NumberAt(1);
			return true;
		}
	}

	return false;
}


// #pragma mark -


BString
CapabilityHandler::CommandString()
{
	return "CAPABILITY";
}


bool
CapabilityHandler::HandleUntagged(Response& response)
{
	if (!response.IsCommand("CAPABILITY"))
		return false;

	fCapabilities.MakeEmpty();
	while (response.CountItems() > 1)
		fCapabilities.AddItem(response.RemoveItemAt(1));

	TRACE("CAPABILITY: %s\n", fCapabilities.ToString().String());
	return true;
}


// #pragma mark -


FetchMessageEntriesCommand::FetchMessageEntriesCommand(
	MessageEntryList& entries, uint32 from, uint32 to)
	:
	fEntries(entries),
	fFrom(from),
	fTo(to)
{
}


BString
FetchMessageEntriesCommand::CommandString()
{
	BString command = "UID FETCH ";
	command << fFrom << ":" << fTo << " (FLAGS RFC822.SIZE)";

	return command;
}


bool
FetchMessageEntriesCommand::HandleUntagged(Response& response)
{
	if (!response.EqualsAt(1, "FETCH") || !response.IsListAt(2))
		return false;

	MessageEntry entry;

	ArgumentList& list = response.ListAt(2);
	for (int32 i = 0; i < list.CountItems(); i += 2) {
		if (list.EqualsAt(i, "UID") && list.IsNumberAt(i + 1))
			entry.uid = list.NumberAt(i + 1);
		else if (list.EqualsAt(i, "RFC822.SIZE") && list.IsNumberAt(i + 1))
			entry.size = list.NumberAt(i + 1);
		else if (list.EqualsAt(i, "FLAGS") && list.IsListAt(i + 1)) {
			// Parse flags
			ArgumentList& flags = list.ListAt(i + 1);
			entry.flags = ParseFlags(flags);
		}
	}

	if (entry.uid == 0)
		return false;

	fEntries.push_back(entry);
	return true;
}


// #pragma mark -


FetchCommand::FetchCommand(uint32 from, uint32 to,
	FetchMode mode)
	:
	fFrom(from),
	fTo(to),
	fMode(mode)
{
}


void
FetchCommand::SetListener(FetchListener* listener)
{
	fListener = listener;
}


BString
FetchCommand::CommandString()
{
	BString command = "UID FETCH ";
	command << fFrom;
	if (fFrom != fTo)
		command << ":" << fTo;

	command += " (UID ";
	switch (fMode) {
		case kFetchHeader:
			command += "RFC822.HEADER";
			break;
		case kFetchBody:
			command += "BODY.PEEK[TEXT]";
			break;
		case kFetchAll:
			command += "BODY.PEEK[]";
			break;
	}
	command += ")";

	return command;
}


bool
FetchCommand::HandleUntagged(Response& response)
{
	if (!response.EqualsAt(1, "FETCH") || !response.IsListAt(2))
		return false;

	// We don't need to parse anything here - all the data is processed via
	// HandleLiteral().
	return true;
}


bool
FetchCommand::HandleLiteral(Response& response, BDataIO& stream,
	size_t length)
{
	if (fListener == NULL || !response.EqualsAt(1, "FETCH")
		|| !response.IsListAt(2))
		return false;

	uint32 uid = 0;

	// Get current UID
	ArgumentList& list = response.ListAt(2);
	for (int32 i = 0; i < list.CountItems(); i += 2) {
		if (list.EqualsAt(i, "UID") && list.IsNumberAt(i + 1)) {
			uid = list.NumberAt(i + 1);
			break;
		}
	}

	if (uid == 0)
		return false;

	return fListener->FetchData(response, uid, fMode, stream, length);
}


// #pragma mark -


SetFlagsCommand::SetFlagsCommand(uint32 uid, uint32 flags)
	:
	fUID(uid),
	fFlags(flags)
{
}


BString
SetFlagsCommand::CommandString()
{
	BString command = "STORE ";
	command << fUID << " FLAGS (" << GenerateFlagString(fFlags) << ")";

	return command;
}


bool
SetFlagsCommand::HandleUntagged(Response& response)
{
	// We're not that interested in the outcome, apparently
	return response.EqualsAt(1, "FETCH");
}


// #pragma mark -


#if 0
AppendCommand::AppendCommand(IMAPMailbox& mailbox, BPositionIO& message,
	off_t size, int32 flags, time_t time)
	:
	IMAPMailboxCommand(mailbox),

	fMessageData(message),
	fDataSize(size),
	fFlags(flags),
	fTime(time)
{
}


BString
AppendCommand::CommandString()
{
	BString command = "APPEND ";
	command << fIMAPMailbox.Mailbox();
	command += " (";
	command += SetFlagsCommand::GenerateFlagList(fFlags);
	command += ")";
	command += " {";
	command << fDataSize;
	command += "}";
	return command;
}


bool
AppendCommand::HandleUntagged(const BString& response)
{
	if (response.FindFirst("+") != 0)
		return false;
	fMessageData.Seek(0, SEEK_SET);

	const int32 kBunchSize = 1024; // 1Kb
	char buffer[kBunchSize];

	int32 writeSize = fDataSize;
	while (writeSize > 0) {
		int32 bunchSize = writeSize < kBunchSize ? writeSize : kBunchSize;
		fMessageData.Read(buffer, bunchSize);
		int nWritten = fIMAPMailbox.SendRawData(buffer, bunchSize);
		if (nWritten < 0)
			return false;
		writeSize -= nWritten;
		TRACE("%i\n", (int)writeSize);
	}

	fIMAPMailbox.SendRawData(CRLF, strlen(CRLF));
	return true;
}
#endif


// #pragma mark -


ExistsHandler::ExistsHandler()
{
}


bool
ExistsHandler::HandleUntagged(Response& response)
{
	if (!response.EqualsAt(1, "EXISTS") || response.IsNumberAt(0))
		return false;

//	int32 expunge = response.NumberAt(0);
#if 0
	Listener().ExistsReceived(expunge);

	if (response.FindFirst("EXISTS") < 0)
		return false;

	int32 exists = 0;
	if (!IMAPParser::ExtractUntagedFromLeft(response, "EXISTS", exists))
		return false;

	int32 nMessages = fIMAPMailbox.GetCurrentMessageCount();
	if (exists <= nMessages)
		return true;

	MinMessageList& list = const_cast<MinMessageList&>(
		fIMAPMailbox.GetMessageList());
	IMAPCommand* command = new FetchMinMessageCommand(fIMAPMailbox,
		nMessages + 1, exists, &list, NULL);
	fIMAPMailbox.AddAfterQuakeCommand(command);

	fIMAPMailbox.Listener().NewMessagesToFetch(exists - nMessages);

	command = new FetchMessageCommand(fIMAPMailbox, nMessages + 1, exists,
		fIMAPMailbox.FetchBodyLimit());
	fIMAPMailbox.AddAfterQuakeCommand(command);

	TRACE("EXISTS %i\n", (int)exists);
	fIMAPMailbox.SendRawCommand("DONE");
#endif

	return true;
}


// #pragma mark -


ExpungeCommand::ExpungeCommand()
{
}


BString
ExpungeCommand::CommandString()
{
	return "EXPUNGE";
}


// #pragma mark -


ExpungeHandler::ExpungeHandler()
{
}


bool
ExpungeHandler::HandleUntagged(Response& response)
{
	if (!response.EqualsAt(1, "EXPUNGE") || response.IsNumberAt(0))
		return false;

//	int32 expunge = response.NumberAt(0);
#if 0
	Listener().ExpungeReceived(expunge);

	// remove from storage
	IMAPStorage& storage = fIMAPMailbox.GetStorage();
	storage.DeleteMessage(fIMAPMailbox.MessageNumberToUID(expunge));

	// remove from min message list
	MinMessageList& messageList = const_cast<MinMessageList&>(
		fIMAPMailbox.GetMessageList());
	messageList.erase(messageList.begin() + expunge - 1);

	TRACE("EXPUNGE %i\n", (int)expunge);

	// the watching loop restarts again, we need to watch again to because
	// some IDLE implementation stop sending notifications
	fIMAPMailbox.SendRawCommand("DONE");
#endif
	return true;
}


// #pragma mark -


#if 0
FlagsHandler::FlagsHandler(IMAPMailbox& mailbox)
	:
	IMAPMailboxCommand(mailbox)
{
}


bool
FlagsHandler::HandleUntagged(const BString& response)
{
	if (response.FindFirst("FETCH") < 0)
		return false;

	int32 fetch = 0;
	if (!IMAPParser::ExtractUntagedFromLeft(response, "FETCH", fetch))
		return false;

	int32 flags = FetchMinMessageCommand::ExtractFlags(response);
	int32 uid = fIMAPMailbox.MessageNumberToUID(fetch);
	fStorage.SetFlags(uid, flags);
	TRACE("FlagsHandler id %i flags %i\n", (int)fetch, (int)flags);
	fIMAPMailbox.SendRawCommand("DONE");

	return true;
}
#endif


// #pragma mark -


ListCommand::ListCommand(const char* prefix, bool subscribedOnly)
	:
	fPrefix(prefix),
	fSubscribedOnly(subscribedOnly)
{
}


BString
ListCommand::CommandString()
{
	BString command = _Command();
	command += " \"\" \"";
	if (fPrefix != NULL)
		command << fEncoding.Encode(fPrefix) << "%";
	else
		command += "*";
	command += "\"";

	return command;
}


bool
ListCommand::HandleUntagged(Response& response)
{
	if (response.IsCommand(_Command()) && response.IsStringAt(3)) {
		BString folder = response.StringAt(3);
		try {
			fFolders.push_back(fEncoding.Decode(folder));
		} catch (ParseException& exception) {
			// Decoding failed, just add the plain text
			fprintf(stderr, "Decoding \"%s\" failed: %s\n", folder.String(),
				exception.Message());
			fFolders.push_back(folder);
		}
		return true;
	}

	return false;
}


const StringList&
ListCommand::FolderList()
{
	return fFolders;
}


const char*
ListCommand::_Command() const
{
	return fSubscribedOnly ? "LSUB" : "LIST";
}


// #pragma mark -


SubscribeCommand::SubscribeCommand(const char* mailboxName)
	:
	fMailboxName(mailboxName)
{
}


BString
SubscribeCommand::CommandString()
{
	BString command = "SUBSCRIBE \"";
	command += fMailboxName;
	command += "\"";
	return command;
}


// #pragma mark -


UnsubscribeCommand::UnsubscribeCommand(const char* mailboxName)
	:
	fMailboxName(mailboxName)
{
}


BString
UnsubscribeCommand::CommandString()
{
	BString command = "UNSUBSCRIBE \"";
	command += fMailboxName;
	command += "\"";
	return command;
}


// #pragma mark -


GetQuotaCommand::GetQuotaCommand(const char* mailboxName)
	:
	fMailboxName(mailboxName),
	fUsedStorage(0),
	fTotalStorage(0)
{
}


BString
GetQuotaCommand::CommandString()
{
	BString command = "GETQUOTAROOT \"";
	command += fMailboxName;
	command += "\"";
	return command;
}


bool
GetQuotaCommand::HandleUntagged(Response& response)
{
	if (!response.IsCommand("QUOTA") || !response.IsListAt(2))
		return false;

	const ArgumentList& arguments = response.ListAt(2);
	if (!arguments.EqualsAt(0, "STORAGE"))
		return false;

	fUsedStorage = (uint64)arguments.NumberAt(1) * 1024;
	fTotalStorage = (uint64)arguments.NumberAt(2) * 1024;

	return true;
}


uint64
GetQuotaCommand::UsedStorage()
{
	return fUsedStorage;
}


uint64
GetQuotaCommand::TotalStorage()
{
	return fTotalStorage;
}


}	// namespace
