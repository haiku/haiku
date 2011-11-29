/*
 * Copyright 2001-2011, Haiku Inc. All Rights Reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2010 Clemens Zeidler. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H


#include <map>

#include <BufferedDataIO.h>
#include <ObjectList.h>
#include <OS.h>
#include <SecureSocket.h>
#include <String.h>

#include "Commands.h"


#define xEOF    236
const bigtime_t kIMAP4ClientTimeout = 1000000 * 60; // 60 sec


namespace IMAP {


class Command;
class Handler;


typedef BObjectList<Command> CommandList;
typedef BObjectList<Handler> HandlerList;
typedef std::map<int32, Command*> CommandIDMap;


// TODO: throw this class away, and just use a BBufferedDataIO instead.
class ConnectionReader {
public:
								ConnectionReader();

			void				SetTo(BSocket& socket);

			/*! Try to read line. If no end of line is found at least
			minUnfinishedLine characters are returned. */
			status_t			GetNextLine(BString& line,
									bigtime_t timeout = kIMAP4ClientTimeout,
									int32 maxUnfinishedLine = -1);
			/*! Read data and append it to line till the end of file is
			reached. */
			status_t			FinishLine(BString& line);

			status_t			ReadToStream(int32 size, BDataIO& out);

private:
			/*! Try to read till the end of line is reached. To do so maximal
			maxNewLength bytes are read from the server if needed. */
			status_t			_GetNextDataBunch(BString& line,
									bigtime_t timeout,
									uint32 maxNewLength = 256);
			bool				_ExtractTillEndOfLine(BString& out);

private:
			BSocket*			fSocket;
			BBufferedDataIO*	fBufferedSocket;
			BString				fStringBuffer;
};


class FolderInfo {
public:
	FolderInfo()
		:
		subscribed(false)
	{
	}

	BString	folder;
	bool	subscribed;
};
typedef std::vector<FolderInfo> FolderList;


class Protocol {
public:
								Protocol();
								~Protocol();

			/*! Indicates that the current action should be interrupted because
			we are going to be deleted. */
			void				SetStopNow();
			bool				StopNow();

			status_t			Connect(const BNetworkAddress& address,
									const char* username, const char* password,
									bool useSSL = true);
			status_t			Disconnect();
			bool				IsConnected();

			status_t			SendCommand(const char* command);
			status_t			SendCommand(int32 id, const char* command);
			ssize_t				SendData(const char* buffer, uint32 length);

			status_t			GetFolders(FolderList& folders);
			status_t			SubscribeFolder(const char* folder);
			status_t			UnsubscribeFolder(const char* folder);

			status_t			SelectMailbox(const char* mailbox);
			const BString&		Mailbox() const { return fMailbox; }

			status_t			GetQuota(uint64& used, uint64& total);

			/*! Install a temporary handler at first position in the handler
			list. */
			status_t			ProcessCommand(Command& command,
									bigtime_t timeout = kIMAP4ClientTimeout);

			ArgumentList&		Capabilities() { return fCapabilities; }
			const ArgumentList&	Capabilities() const { return fCapabilities; }

			status_t			AddAfterQuakeCommand(Command* command);

			const BString&		CommandError() { return fCommandError; }

protected:
			status_t			HandleResponse(
									bigtime_t timeout = kIMAP4ClientTimeout,
									bool disconnectOnTimeout = true);
			void				ProcessAfterQuacks(bigtime_t timeout);
			int32				NextCommandID();

private:
			/*! Same as ProccessCommand but AfterShockCommands are not send. */
			status_t			_ProcessCommandWithoutAfterQuake(
									Command& command, bigtime_t timeout);
			status_t			_Disconnect();
			status_t			_GetAllFolders(StringList& folders);
			status_t			_GetSubscribedFolders(StringList& folders);
			void				_ParseCapabilities(
									const ArgumentList& arguments);

protected:
			BSocket* 			fSocket;
			BBufferedDataIO*	fBufferedSocket;
			ConnectionReader	fConnectionReader;

			HandlerList			fHandlerList;
			CommandList			fAfterQuackCommands;
			ArgumentList		fCapabilities;

private:
			BString				fMailbox;
			int32				fCommandID;
			CommandIDMap		fOngoingCommands;
			BString				fCommandError;

			vint32				fStopNow;
			bool				fIsConnected;
};


}	// namespace IMAP


#endif // PROTOCOL_H
