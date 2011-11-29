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


namespace IMAP {


static HandlerListener sEmptyHandler;


Handler::Handler()
	:
	fListener(&sEmptyHandler)
{
}


Handler::~Handler()
{
}


void
Handler::SetListener(HandlerListener& listener)
{
	fListener = &listener;
}


IMAP::LiteralHandler*
Handler::LiteralHandler()
{
	return NULL;
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


void
HandlerListener::FetchBody(Command& command, int32 size)
{
}


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
	command << fFrom << ":" << fTo << " FLAGS";

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
		else if (list.EqualsAt(i, "FLAGS") && list.IsListAt(i + 1)) {
			// Parse flags
			ArgumentList& flags = list.ListAt(i + 1);
			printf("flags: %s\n", flags.ToString().String());
			for (int32 j = 0; j < flags.CountItems(); j++) {
				if (flags.EqualsAt(j, "\\Seen"))
					entry.flags |= kSeen;
				else if (flags.EqualsAt(j, "\\Answered"))
					entry.flags |= kAnswered;
				else if (flags.EqualsAt(j, "\\Flagged"))
					entry.flags |= kFlagged;
				else if (flags.EqualsAt(j, "\\Deleted"))
					entry.flags |= kDeleted;
				else if (flags.EqualsAt(j, "\\Draft"))
					entry.flags |= kDraft;
			}
		}
	}

	if (entry.uid == 0)
		return false;

	fEntries.push_back(entry);
	return true;
}


// #pragma mark -


#if 0
FetchMinMessageCommand::FetchMinMessageCommand(IMAPMailbox& mailbox,
	int32 message, MinMessageList* list, BPositionIO** data)
	:
	IMAPMailboxCommand(mailbox),

	fMessage(message),
	fEndMessage(-1),
	fMinMessageList(list),
	fData(data)
{
}


FetchMinMessageCommand::FetchMinMessageCommand(IMAPMailbox& mailbox,
	int32 firstMessage, int32 lastMessage, MinMessageList* list,
	BPositionIO** data)
	:
	IMAPMailboxCommand(mailbox),

	fMessage(firstMessage),
	fEndMessage(lastMessage),
	fMinMessageList(list),
	fData(data)
{
}


BString
FetchMinMessageCommand::CommandString()
{
	if (fMessage <= 0)
		return "";
	BString command = "FETCH ";
	command << fMessage;
	if (fEndMessage > 0) {
		command += ":";
		command << fEndMessage;
	}
	command += " (UID FLAGS)";
	return command;
}


bool
FetchMinMessageCommand::HandleUntagged(const BString& response)
{
	BString extracted = response;
	int32 message;
	if (!IMAPParser::RemoveUntagedFromLeft(extracted, "FETCH", message))
		return false;

	// check if we requested this message
	int32 end = message;
	if (fEndMessage > 0)
		end = fEndMessage;
	if (message < fMessage && message > end)
		return false;

	MinMessage minMessage;
	if (!ParseMinMessage(extracted, minMessage))
		return false;

	fMinMessageList->push_back(minMessage);
	fStorage.AddNewMessage(minMessage.uid, minMessage.flags, fData);
	return true;
}


bool
FetchMinMessageCommand::ParseMinMessage(const BString& response,
	MinMessage& minMessage)
{
	BString extracted = IMAPParser::ExtractNextElement(response);
	BString uid = IMAPParser::ExtractElementAfter(extracted, "UID");
	if (uid == "")
		return false;
	minMessage.uid = atoi(uid);

	int32 flags = ExtractFlags(extracted);
	minMessage.flags = flags;

	return true;
}


int32
FetchMinMessageCommand::ExtractFlags(const BString& response)
{
	int32 flags = 0;
	BString flagsString = IMAPParser::ExtractElementAfter(response, "FLAGS");

	while (true) {
		BString flag = IMAPParser::RemovePrimitiveFromLeft(flagsString);
		if (flag == "")
			break;

		if (flag == "\\Seen")
			flags |= kSeen;
		else if (flag == "\\Answered")
			flags |= kAnswered;
		else if (flag == "\\Flagged")
			flags |= kFlagged;
		else if (flag == "\\Deleted")
			flags |= kDeleted;
		else if (flag == "\\Draft")
			flags |= kDraft;
	}
	return flags;
}


// #pragma mark -


FetchMessageCommand::FetchMessageCommand(IMAPMailbox& mailbox, int32 message,
	BPositionIO* data, int32 fetchBodyLimit)
	:
	IMAPMailboxCommand(mailbox),

	fMessage(message),
	fEndMessage(-1),
	fOutData(data),
	fFetchBodyLimit(fetchBodyLimit)
{
}


FetchMessageCommand::FetchMessageCommand(IMAPMailbox& mailbox,
	int32 firstMessage, int32 lastMessage, int32 fetchBodyLimit)
	:
	IMAPMailboxCommand(mailbox),

	fMessage(firstMessage),
	fEndMessage(lastMessage),
	fOutData(NULL),
	fFetchBodyLimit(fetchBodyLimit)
{
	if (fEndMessage > 0)
		fUnhandled = fEndMessage - fMessage + 1;
	else
		fUnhandled = 1;
}


FetchMessageCommand::~FetchMessageCommand()
{
	for (int32 i = 0; i < fUnhandled; i++)
		fIMAPMailbox.Listener().FetchEnd();
}


BString
FetchMessageCommand::CommandString()
{
	BString command = "FETCH ";
	command << fMessage;
	if (fEndMessage > 0) {
		command += ":";
		command << fEndMessage;
	}
	command += " (RFC822.SIZE RFC822.HEADER)";
	return command;
}


bool
FetchMessageCommand::HandleUntagged(const BString& response)
{
	BString extracted = response;
	int32 message;
	if (!IMAPParser::RemoveUntagedFromLeft(extracted, "FETCH", message))
		return false;

	// check if we requested this message
	int32 end = message;
	if (fEndMessage > 0)
		end = fEndMessage;
	if (message < fMessage && message > end)
		return false;

	const MinMessageList& list = fIMAPMailbox.GetMessageList();
	int32 index = message - 1;
	if (index < 0 || index >= (int32)list.size())
		return false;
	const MinMessage& minMessage = list[index];

	BPositionIO* data = fOutData;
	ObjectDeleter<BPositionIO> deleter;
	if (!data) {
		status_t status = fStorage.OpenMessage(minMessage.uid, &data);
		if (status != B_OK) {
			status = fStorage.AddNewMessage(minMessage.uid, minMessage.flags,
				&data);
		}
		if (status != B_OK)
			return false;
		deleter.SetTo(data);
	}

	// read message size
	BString messageSizeString = IMAPParser::ExtractElementAfter(extracted,
		"RFC822.SIZE");
	int32 messageSize = atoi(messageSizeString);
	fStorage.SetCompleteMessageSize(minMessage.uid, messageSize);

	// read header
	int32 headerPos = extracted.FindFirst("RFC822.HEADER");
	if (headerPos < 0) {
		if (!fOutData)
			fStorage.DeleteMessage(minMessage.uid);
		return false;
	}
	extracted.Remove(0, headerPos + strlen("RFC822.HEADER") + 1);
	BString headerSize = IMAPParser::RemovePrimitiveFromLeft(extracted);
	headerSize = IMAPParser::ExtractNextElement(headerSize);
	int32 size = atoi(headerSize);

	status_t status = fConnectionReader.ReadToFile(size, data);
	if (status != B_OK) {
		if (!fOutData)
			fStorage.DeleteMessage(minMessage.uid);
		return false;
	}

	// read last ")" line
	BString lastLine;
	fConnectionReader.GetNextLine(lastLine);

	fUnhandled--;

	bool bodyIsComing = true;
	if (fFetchBodyLimit >= 0 && fFetchBodyLimit <= messageSize)
		bodyIsComing = false;

	int32 uid = fIMAPMailbox.MessageNumberToUID(message);
	if (uid >= 0)
		fIMAPMailbox.Listener().HeaderFetched(uid, data, bodyIsComing);

	if (!bodyIsComing)
		return true;

	deleter.Detach();
	FetchBodyCommand* bodyCommand = new FetchBodyCommand(fIMAPMailbox, message,
		data);
	fIMAPMailbox.AddAfterQuakeCommand(bodyCommand);

	return true;
}


// #pragma mark -


FetchBodyCommand::FetchBodyCommand(IMAPMailbox& mailbox, int32 message,
	BPositionIO* data)
	:
	IMAPMailboxCommand(mailbox),

	fMessage(message),
	fOutData(data)
{
}


FetchBodyCommand::~FetchBodyCommand()
{
	delete fOutData;
}


BString
FetchBodyCommand::CommandString()
{
	BString command = "FETCH ";
	command << fMessage;
	command += " (FLAGS BODY.PEEK[TEXT])";
	return command;
}


bool
FetchBodyCommand::HandleUntagged(const BString& response)
{
	if (response.FindFirst("FETCH") < 0)
		return false;

	BString extracted = response;
	int32 message;
	if (!IMAPParser::RemoveUntagedFromLeft(extracted, "FETCH", message))
		return false;
	if (message != fMessage)
		return false;

	int32 flags = FetchMinMessageCommand::ExtractFlags(extracted);
	fStorage.SetFlags(fIMAPMailbox.MessageNumberToUID(message), flags);

	int32 textPos = extracted.FindFirst("BODY[TEXT]");
	if (textPos < 0)
		return false;
	extracted.Remove(0, textPos + strlen("BODY[TEXT]") + 1);
	BString bodySize = IMAPParser::ExtractBetweenBrackets(extracted, "{", "}");
	bodySize = IMAPParser::ExtractNextElement(bodySize);
	int32 size = atoi(bodySize);
	TRACE("Body size %i\n", (int)size);
	fOutData->Seek(0, SEEK_END);
	status_t status = fConnectionReader.ReadToFile(size, fOutData);
	if (status != B_OK)
		return false;

	// read last ")" line
	BString lastLine;
	fConnectionReader.GetNextLine(lastLine);

	int32 uid = fIMAPMailbox.MessageNumberToUID(message);
	if (uid >= 0)
		fIMAPMailbox.Listener().BodyFetched(uid, fOutData);
	else
		fIMAPMailbox.Listener().FetchEnd();

	return true;
}


// #pragma mark -


SetFlagsCommand::SetFlagsCommand(IMAPMailbox& mailbox, int32 message,
	int32 flags)
	:
	IMAPMailboxCommand(mailbox),

	fMessage(message),
	fFlags(flags)
{
}


BString
SetFlagsCommand::CommandString()
{
	BString command = "STORE ";
	command << fMessage;
	command += " FLAGS (";
	command += GenerateFlagList(fFlags);
	command += ")";
	return command;
}


bool
SetFlagsCommand::HandleUntagged(const BString& response)
{
	return false;
}


BString
SetFlagsCommand::GenerateFlagList(int32 flags)
{
	BString flagList;

	if ((flags & kSeen) != 0)
		flagList += "\\Seen ";
	if ((flags & kAnswered) != 0)
		flagList += "\\Answered ";
	if ((flags & kFlagged) != 0)
		flagList += "\\Flagged ";
	if ((flags & kDeleted) != 0)
		flagList += "\\Deleted ";
	if ((flags & kDraft) != 0)
		flagList += "\\Draft ";

	return flagList.Trim();
}


// #pragma mark -


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

	int32 expunge = response.NumberAt(0);
	Listener().ExistsReceived(expunge);

#if 0
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

	int32 expunge = response.NumberAt(0);
	Listener().ExpungeReceived(expunge);

#if 0
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


BString
ListCommand::CommandString()
{
	fFolders.clear();
	return "LIST \"\" \"*\"";
}


bool
ListCommand::HandleUntagged(Response& response)
{
	if (response.IsCommand("LIST") && response.IsStringAt(3)) {
		fFolders.push_back(response.StringAt(3));
		return true;
	}

	return false;
}


const StringList&
ListCommand::FolderList()
{
	return fFolders;
}


// #pragma mark -


BString
ListSubscribedCommand::CommandString()
{
	fFolders.clear();
	return "LSUB \"\" \"*\"";
}


bool
ListSubscribedCommand::HandleUntagged(Response& response)
{
	if (response.IsCommand("LSUB") && response.IsStringAt(3)) {
		fFolders.push_back(response.StringAt(3));
		return true;
	}

	return false;
}


const StringList&
ListSubscribedCommand::FolderList()
{
	return fFolders;
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
	BString command = "GETQUOTA \"";
	command += fMailboxName;
	command += "\"";
	return command;
}


bool
GetQuotaCommand::HandleUntagged(Response& response)
{
	if (!response.IsCommand("QUOTA") || response.IsListAt(1))
		return false;

	const ArgumentList& arguments = response.ListAt(1);
	fUsedStorage = (uint64)arguments.NumberAt(0) * 1024;
	fTotalStorage = (uint64)arguments.NumberAt(1) * 1024;

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
