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


struct MessageEntry {
	MessageEntry()
		:
		uid(0),
		flags(0)
	{
	}

	uint32	uid;
	uint32	flags;
	uint32	size;
};
typedef std::vector<MessageEntry> MessageEntryList;

enum MessageFlags {
	kSeen		= 0x01,
	kAnswered	= 0x02,
	kFlagged	= 0x04,
	kDeleted	= 0x08,
	kDraft		= 0x10
	// \Recent doesn't really have any useful meaning, so we just ignore it
};


class Handler {
public:
								Handler();
	virtual						~Handler();

	virtual	bool				HandleUntagged(Response& response) = 0;
};


class Command {
public:
	virtual						~Command();

	virtual	BString				CommandString() = 0;
	virtual	status_t			HandleTagged(Response& response);
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

	virtual	BString				CommandString();
	virtual	bool				HandleUntagged(Response& response);

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


enum FetchMode {
	kFetchHeader,
	kFetchBody,
	kFetchAll
};


class FetchListener {
public:
	virtual	bool				FetchData(Response& reponse, uint32 uid,
									FetchMode mode, BDataIO& stream,
									size_t length) = 0;
};


class FetchCommand : public Command, public Handler,
	public LiteralHandler {
public:
								FetchCommand(uint32 from, uint32 to,
									FetchMode mode);

			void				SetListener(FetchListener* listener);
			FetchListener*		Listener() const { return fListener; }

	virtual	BString				CommandString();
	virtual	bool				HandleUntagged(Response& response);
	virtual bool				HandleLiteral(Response& response,
									BDataIO& stream, size_t length);

private:
			uint32				fFrom;
			uint32				fTo;
			FetchMode			fMode;
			FetchListener*		fListener;
};


class SetFlagsCommand : public Command, public Handler {
public:
								SetFlagsCommand(uint32 uid, uint32 flags);

	virtual	BString				CommandString();
	virtual	bool				HandleUntagged(Response& response);

private:
			uint32				fUID;
			uint32				fFlags;
};


#if 0
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
								ListCommand(const char* prefix,
									bool subscribedOnly);

			BString				CommandString();
			bool				HandleUntagged(Response& response);

	const	StringList&			FolderList();

private:
			const char*			_Command() const;

private:
			RFC3501Encoding		fEncoding;
			const char*			fPrefix;
			StringList			fFolders;
			bool				fSubscribedOnly;
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
