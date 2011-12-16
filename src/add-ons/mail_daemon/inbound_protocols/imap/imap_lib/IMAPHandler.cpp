/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "IMAPHandler.h"

#include <stdlib.h>

#include <AutoDeleter.h>

#include "IMAPMailbox.h"
#include "IMAPParser.h"
#include "IMAPStorage.h"


#define DEBUG_IMAP_HANDLER
#ifdef DEBUG_IMAP_HANDLER
#	include <stdio.h>
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) ;
#endif


using namespace BPrivate;


IMAPCommand::~IMAPCommand()
{
}


BString
IMAPCommand::Command()
{
	return "";
}


IMAPMailboxCommand::IMAPMailboxCommand(IMAPMailbox& mailbox)
	:
	fIMAPMailbox(mailbox),
	fStorage(mailbox.GetStorage()),
	fConnectionReader(mailbox.GetConnectionReader())
{
}


IMAPMailboxCommand::~IMAPMailboxCommand()
{
}


// #pragma mark -


MailboxSelectHandler::MailboxSelectHandler(IMAPMailbox& mailbox)
	:
	IMAPMailboxCommand(mailbox),

	fMailboxName(""),
	fNextUID(-1),
	fUIDValidity(-1)
{
}


BString
MailboxSelectHandler::Command()
{
	if (fMailboxName == "")
		return "";

	BString command = "SELECT \"";
	command += fMailboxName;
	command += "\"";
	return command;
}


bool
MailboxSelectHandler::Handle(const BString& response)
{
	BString extracted = IMAPParser::ExtractStringAfter(response,
		"* OK [UIDVALIDITY");
	if (extracted != "") {
		fUIDValidity = IMAPParser::RemoveIntegerFromLeft(extracted);
		TRACE("UIDValidity %i\n", (int)fUIDValidity);
		return true;
	}

	extracted = IMAPParser::ExtractStringAfter(response, "* OK [UIDNEXT");
	if (extracted != "") {
		fNextUID = IMAPParser::RemoveIntegerFromLeft(extracted);
		TRACE("NextUID %i\n", (int)fNextUID);
		return true;
	}

	return false;
}


// #pragma mark -


CapabilityHandler::CapabilityHandler()
	:
	fCapabilities("")
{
}


BString
CapabilityHandler::Command()
{
	return "CAPABILITY";
}


bool
CapabilityHandler::Handle(const BString& response)
{
	BString cap = IMAPParser::ExtractStringAfter(response, "* CAPABILITY");
	if (cap == "")
		return false;
	fCapabilities = cap;
	TRACE("CAPABILITY: %s\n", fCapabilities.String());
	return true;
}


BString&
CapabilityHandler::Capabilities()
{
	return fCapabilities;
}


// #pragma mark -


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
FetchMinMessageCommand::Command()
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
FetchMinMessageCommand::Handle(const BString& response)
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


FetchMessageListCommand::FetchMessageListCommand(IMAPMailbox& mailbox,
	MinMessageList* list, int32 nextId)
	:
	IMAPMailboxCommand(mailbox),

	fMinMessageList(list),
	fNextId(nextId)
{
}


BString
FetchMessageListCommand::Command()
{
	BString command = "UID FETCH 1:";
	command << fNextId - 1;
	command << " FLAGS";
	return command;
}


bool
FetchMessageListCommand::Handle(const BString& response)
{
	BString extracted = response;
	int32 message;
	if (!IMAPParser::RemoveUntagedFromLeft(extracted, "FETCH", message))
		return false;

	MinMessage minMessage;
	if (!FetchMinMessageCommand::ParseMinMessage(extracted, minMessage))
		return false;

	fMinMessageList->push_back(minMessage);
	return true;
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
FetchMessageCommand::Command()
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
FetchMessageCommand::Handle(const BString& response)
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
FetchBodyCommand::Command()
{
	BString command = "FETCH ";
	command << fMessage;
	command += " (FLAGS BODY.PEEK[TEXT])";
	return command;
}


bool
FetchBodyCommand::Handle(const BString& response)
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
SetFlagsCommand::Command()
{
	BString command = "STORE ";
	command << fMessage;
	command += " FLAGS (";
	command += GenerateFlagList(fFlags);
	command += ")";
	return command;
}


bool
SetFlagsCommand::Handle(const BString& response)
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
AppendCommand::Command()
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
AppendCommand::Handle(const BString& response)
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


// #pragma mark -


ExistsHandler::ExistsHandler(IMAPMailbox& mailbox)
	:
	IMAPMailboxCommand(mailbox)
{
}


bool
ExistsHandler::Handle(const BString& response)
{
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

	return true;
}


// #pragma mark -


ExpungeCommmand::ExpungeCommmand(IMAPMailbox& mailbox)
	:
	IMAPMailboxCommand(mailbox)
{
}


BString
ExpungeCommmand::Command()
{
	return "EXPUNGE";
}


bool
ExpungeCommmand::Handle(const BString& response)
{
	return false;
}


// #pragma mark -


ExpungeHandler::ExpungeHandler(IMAPMailbox& mailbox)
	:
	IMAPMailboxCommand(mailbox)
{
}


bool
ExpungeHandler::Handle(const BString& response)
{
	if (response.FindFirst("EXPUNGE") < 0)
		return false;

	int32 expunge = 0;
	if (!IMAPParser::ExtractUntagedFromLeft(response, "EXPUNGE", expunge))
		return false;

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
	return true;
}


// #pragma mark -


FlagsHandler::FlagsHandler(IMAPMailbox& mailbox)
	:
	IMAPMailboxCommand(mailbox)
{
}


bool
FlagsHandler::Handle(const BString& response)
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


// #pragma mark -


BString
ListCommand::Command()
{
	fFolders.clear();
	return "LIST \"\" \"*\"";
}


bool
ListCommand::Handle(const BString& response)
{
	return ParseList("LIST", response, fFolders);
}


const StringList&
ListCommand::FolderList()
{
	return fFolders;
}


bool
ListCommand::ParseList(const char* command, const BString& response,
	StringList& list)
{
	int32 textPos = response.FindFirst(command);
	if (textPos < 0)
		return false;
	BString extracted = response;

	extracted.Remove(0, textPos + strlen(command) + 1);
	extracted.Trim();
	if (extracted[0] == '(') {
		BString flags = IMAPParser::ExtractBetweenBrackets(extracted, "(", ")");
		if (flags.IFindFirst("\\Noselect") >= 0)
			return true;
		textPos = extracted.FindFirst(")");
		extracted.Remove(0, textPos + 1);
	}

	IMAPParser::RemovePrimitiveFromLeft(extracted);
	extracted.Trim();
	// remove quotation marks
	extracted.Remove(0, 1);
	extracted.Truncate(extracted.Length() - 1);

	list.push_back(extracted);
	return true;
}


// #pragma mark -


BString
ListSubscribedCommand::Command()
{
	fFolders.clear();
	return "LSUB \"\" \"*\"";
}


bool
ListSubscribedCommand::Handle(const BString& response)
{
	return ListCommand::ParseList("LSUB", response, fFolders);
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
SubscribeCommand::Command()
{
	BString command = "SUBSCRIBE \"";
	command += fMailboxName;
	command += "\"";
	return command;
}


bool
SubscribeCommand::Handle(const BString& response)
{
	return false;
}


// #pragma mark -


UnsubscribeCommand::UnsubscribeCommand(const char* mailboxName)
	:
	fMailboxName(mailboxName)
{
}


BString
UnsubscribeCommand::Command()
{
	BString command = "UNSUBSCRIBE \"";
	command += fMailboxName;
	command += "\"";
	return command;
}


bool
UnsubscribeCommand::Handle(const BString& response)
{
	return false;
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
GetQuotaCommand::Command()
{
	BString command = "GETQUOTA \"";
	command += fMailboxName;
	command += "\"";
	return command;
}


bool
GetQuotaCommand::Handle(const BString& response)
{
	if (response.FindFirst("QUOTA") < 0)
		return false;

	BString data = IMAPParser::ExtractBetweenBrackets(response, "(", ")");
	IMAPParser::RemovePrimitiveFromLeft(data);
	fUsedStorage = (uint64)IMAPParser::RemoveIntegerFromLeft(data) * 1024;
	fTotalStorage = (uint64)IMAPParser::RemoveIntegerFromLeft(data) * 1024;

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
