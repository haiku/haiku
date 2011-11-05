/*
 * Copyright 2010-2011, Haiku Inc. All Rights Reserved.
 * Copyright 2010 Clemens Zeidler. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_HANDLER_H
#define IMAP_HANDLER_H


#include <vector>

#include <DataIO.h>
#include <String.h>


class ConnectionReader;
class IMAPMailbox;
class IMAPStorage;


class IMAPCommand {
public:
	virtual						~IMAPCommand();

	virtual	BString				Command();
	virtual bool				Handle(const BString& response) = 0;
};


class IMAPMailboxCommand  : public IMAPCommand{
public:
								IMAPMailboxCommand(IMAPMailbox& mailbox);
	virtual						~IMAPMailboxCommand();

protected:
			IMAPMailbox&		fIMAPMailbox;
			IMAPStorage&		fStorage;
			ConnectionReader&	fConnectionReader;
};


class MailboxSelectHandler : public IMAPMailboxCommand {
public:
								MailboxSelectHandler(IMAPMailbox& mailbox);

			BString				Command();
			bool				Handle(const BString& response);

			void				SetTo(const char* mailboxName)
									{ fMailboxName = mailboxName; }
			int32				NextUID() { return fNextUID; }
			int32				UIDValidity() { return fUIDValidity; }

private:
			BString				fMailboxName;
			int32				fNextUID;
			int32				fUIDValidity;
};


class CapabilityHandler : public IMAPCommand {
public:
								CapabilityHandler();

			BString				Command();
			bool				Handle(const BString& response);

			BString&			Capabilities();

private:
			BString				fCapabilities;
};


struct MinMessage {
	MinMessage();

	int32	uid;
	int32	flags;
};


typedef std::vector<MinMessage>	MinMessageList;


class FetchMinMessageCommand : public IMAPMailboxCommand {
public:
								FetchMinMessageCommand(IMAPMailbox& mailbox,
									int32 message, MinMessageList* list,
									BPositionIO** data);
								FetchMinMessageCommand(IMAPMailbox& mailbox,
									int32 firstMessage, int32 lastMessage,
									MinMessageList* list, BPositionIO** data);

			BString				Command();
			bool				Handle(const BString& response);

	static	bool				ParseMinMessage(const BString& response,
									MinMessage& minMessage);
	static	int32				ExtractFlags(const BString& response);

private:
			int32				fMessage;
			int32				fEndMessage;
			MinMessageList*		fMinMessageList;
			BPositionIO**		fData;
};


class FetchMessageListCommand : public IMAPMailboxCommand {
public:
								FetchMessageListCommand(IMAPMailbox& mailbox,
									MinMessageList* list, int32 nextId);

			BString				Command();
			bool				Handle(const BString& response);

private:
			MinMessageList*		fMinMessageList;
			int32				fNextId;
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

			BString				Command();
			bool				Handle(const BString& response);

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

			BString				Command();
			bool				Handle(const BString& response);

private:
			int32				fMessage;
			BPositionIO*		fOutData;
};


class SetFlagsCommand : public IMAPMailboxCommand {
public:
								SetFlagsCommand(IMAPMailbox& mailbox,
									int32 message, int32 flags);

			BString				Command();
			bool				Handle(const BString& response);

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

			BString				Command();
			bool				Handle(const BString& response);

private:
			BPositionIO&		fMessageData;
			off_t				fDataSize;
			int32				fFlags;
			time_t				fTime;
};


class ExistsHandler : public IMAPMailboxCommand {
public:
								ExistsHandler(IMAPMailbox& mailbox);

			bool				Handle(const BString& response);
};


/*! Just send a expunge command to delete kDeleted flagged messages. The
response is handled by the unsolicited ExpungeHandler which is installed all
the time. */
class ExpungeCommmand : public IMAPMailboxCommand {
public:
								ExpungeCommmand(IMAPMailbox& mailbox);

			BString				Command();
			bool				Handle(const BString& response);
};


class ExpungeHandler : public IMAPMailboxCommand {
public:
								ExpungeHandler(IMAPMailbox& mailbox);

			bool				Handle(const BString& response);
};


class FlagsHandler : public IMAPMailboxCommand {
public:
								FlagsHandler(IMAPMailbox& mailbox);

			bool				Handle(const BString& response);
};


typedef std::vector<BString> StringList;


class ListCommand : public IMAPCommand {
public:
			BString				Command();
			bool				Handle(const BString& response);

	const	StringList&			FolderList();

	static	bool				ParseList(const char* command,
									const BString& response, StringList& list);
private:
			StringList			fFolders;
};


class ListSubscribedCommand : public IMAPCommand {
public:
			BString				Command();
			bool				Handle(const BString& response);

	const	StringList&			FolderList();

private:
			StringList			fFolders;
};


class SubscribeCommand : public IMAPCommand {
public:
								SubscribeCommand(const char* mailboxName);

			BString				Command();
			bool				Handle(const BString& response);

private:
			BString				fMailboxName;
};


class UnsubscribeCommand : public IMAPCommand {
public:
								UnsubscribeCommand(const char* mailboxName);

			BString				Command();
			bool				Handle(const BString& response);

private:
			BString				fMailboxName;
};


class GetQuotaCommand : public IMAPCommand {
public:
								GetQuotaCommand(const char* mailboxName = "");

			BString				Command();
			bool				Handle(const BString& response);

			uint64				UsedStorage();
			uint64				TotalStorage();
private:
			BString				fMailboxName;

			uint64				fUsedStorage;
			uint64				fTotalStorage;
};


#endif // IMAP_HANDLER_H
