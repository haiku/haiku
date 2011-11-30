/*
 * Copyright 2010-2011, Haiku Inc. All Rights Reserved.
 * Copyright 2010 Clemens Zeidler. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef COMMANDS_H
#define COMMANDS_H


#include <vector>

#include "Response.h"


typedef std::vector<BString> StringList;


namespace IMAP {


class HandlerListener;


struct MessageEntry {
	MessageEntry()
		:
		uid(0),
		flags(0)
	{
	}

	uint32	uid;
	uint32	flags;
};
typedef std::vector<MessageEntry> MessageEntryList;

enum MessageFlags {
	kSeen		= 0x01,
	kAnswered	= 0x02,
	kFlagged	= 0x04,
	kDeleted	= 0x08,
	kDraft		= 0x10
};


class Handler {
public:
								Handler();
	virtual						~Handler();

			void				SetListener(HandlerListener& listener);
			HandlerListener&	Listener() { return *fListener; }

	virtual	bool				HandleUntagged(Response& response) = 0;
	virtual IMAP::LiteralHandler* LiteralHandler();

protected:
			HandlerListener*	fListener;
};


class Command {
public:
	virtual						~Command();

	virtual	BString				CommandString() = 0;
	virtual	status_t			HandleTagged(Response& response);
};


class HandlerListener {
public:
	virtual						~HandlerListener();

	virtual	void				ExpungeReceived(int32 number);
	virtual void				ExistsReceived(int32 number);
	virtual void				FetchBody(Command& command, int32 size);
};


class RawCommand : public Command {
public:
								RawCommand(const BString& command);

	virtual	BString				CommandString();

private:
			BString				fCommand;
};


class LoginCommand : public Command, public Handler {
public:
								LoginCommand(const char* user,
									const char* password);

	virtual	BString				CommandString();
	virtual	bool				HandleUntagged(Response& response);

			const ArgumentList&	Capabilities() const { return fCapabilities; }

private:
			const char*			fUser;
			const char*			fPassword;
			ArgumentList		fCapabilities;
};


class SelectCommand : public Command, public Handler {
public:
								SelectCommand();
								SelectCommand(const char* mailboxName);

			BString				CommandString();
			bool				HandleUntagged(Response& response);

			void				SetTo(const char* mailboxName)
									{ fMailboxName = mailboxName; }
			uint32				NextUID() { return fNextUID; }
			uint32				UIDValidity() { return fUIDValidity; }

private:
			BString				fMailboxName;
			uint32				fNextUID;
			uint32				fUIDValidity;
};


class CapabilityHandler : public Command, public Handler {
public:
	virtual	BString				CommandString();
	virtual	bool				HandleUntagged(Response& response);

			const ArgumentList&	Capabilities() const { return fCapabilities; }

private:
			ArgumentList		fCapabilities;
};


class FetchMessageEntriesCommand : public Command, public Handler {
public:
								FetchMessageEntriesCommand(
									MessageEntryList& entries, uint32 from,
									uint32 to);

			BString				CommandString();
	virtual	bool				HandleUntagged(Response& response);

private:
			MessageEntryList&	fEntries;
			uint32				fFrom;
			uint32				fTo;
};

#if 0


class FetchMinMessageCommand : public IMAPMailboxCommand {
public:
								FetchMinMessageCommand(IMAPMailbox& mailbox,
									int32 message, MinMessageList* list,
									BPositionIO** data);
								FetchMinMessageCommand(IMAPMailbox& mailbox,
									int32 firstMessage, int32 lastMessage,
									MinMessageList* list, BPositionIO** data);

			BString				CommandString();
			bool				HandleUntagged(const BString& response);

	static	bool				ParseMinMessage(const BString& response,
									MinMessage& minMessage);
	static	int32				ExtractFlags(const BString& response);

private:
			int32				fMessage;
			int32				fEndMessage;
			MinMessageList*		fMinMessageList;
			BPositionIO**		fData;
};


class FetchMessageCommand : public IMAPMailboxCommand {
public:
								FetchMessageCommand(IMAPMailbox& mailbox,
									int32 message, BPositionIO* data,
									int32 fetchBodyLimit = -1);
			/*! Fetch multiple message within a range. */
								FetchMessageCommand(IMAPMailbox& mailbox,
									int32 firstMessage, int32 lastMessage,
									int32 fetchBodyLimit = -1);
								~FetchMessageCommand();

			BString				CommandString();
			bool				HandleUntagged(const BString& response);

private:
			int32				fMessage;
			int32				fEndMessage;
			BPositionIO*		fOutData;
			int32				fFetchBodyLimit;
			int32				fUnhandled;
};


class FetchBodyCommand : public IMAPMailboxCommand {
public:
								/*! takes ownership of the data */
								FetchBodyCommand(IMAPMailbox& mailbox,
									int32 message, BPositionIO* data);
								~FetchBodyCommand();

			BString				CommandString();
			bool				HandleUntagged(const BString& response);

private:
			int32				fMessage;
			BPositionIO*		fOutData;
};


class SetFlagsCommand : public IMAPMailboxCommand {
public:
								SetFlagsCommand(IMAPMailbox& mailbox,
									int32 message, int32 flags);

			BString				CommandString();
			bool				HandleUntagged(const BString& response);

	static	BString				GenerateFlagList(int32 flags);

private:
			int32				fMessage;
			int32				fFlags;
};


class AppendCommand : public IMAPMailboxCommand {
public:
								AppendCommand(IMAPMailbox& mailbox,
									BPositionIO& message, off_t size,
									int32 flags, time_t time);

			BString				CommandString();
			bool				HandleUntagged(const BString& response);

private:
			BPositionIO&		fMessageData;
			off_t				fDataSize;
			int32				fFlags;
			time_t				fTime;
};
#endif


class ExistsHandler : public Handler {
public:
								ExistsHandler();

			bool				HandleUntagged(Response& response);
};


/*! Just send a expunge command to delete kDeleted flagged messages. The
	response is handled by the unsolicited ExpungeHandler which is installed
	all the time.
*/
class ExpungeCommand : public Command {
public:
								ExpungeCommand();

			BString				CommandString();
};


class ExpungeHandler : public Handler {
public:
								ExpungeHandler();

			bool				HandleUntagged(Response& response);
};


#if 0
class FlagsHandler : public Handler {
public:
								FlagsHandler(IMAPMailbox& mailbox);

			bool				HandleUntagged(const BString& response);
};
#endif


class ListCommand : public Command, public Handler {
public:
			BString				CommandString();
			bool				HandleUntagged(Response& response);

	const	StringList&			FolderList();

private:
			StringList			fFolders;
};


class ListSubscribedCommand : public Command, public Handler {
public:
			BString				CommandString();
			bool				HandleUntagged(Response& response);

	const	StringList&			FolderList();

private:
			StringList			fFolders;
};


class SubscribeCommand : public Command {
public:
								SubscribeCommand(const char* mailboxName);

			BString				CommandString();

private:
			BString				fMailboxName;
};


class UnsubscribeCommand : public Command {
public:
								UnsubscribeCommand(const char* mailboxName);

			BString				CommandString();

private:
			BString				fMailboxName;
};


class GetQuotaCommand : public Command, public Handler {
public:
								GetQuotaCommand(const char* mailboxName = "");

			BString				CommandString();
			bool				HandleUntagged(Response& response);

			uint64				UsedStorage();
			uint64				TotalStorage();
private:
			BString				fMailboxName;

			uint64				fUsedStorage;
			uint64				fTotalStorage;
};


}	// namespace IMAP


#endif // COMMANDS_H
