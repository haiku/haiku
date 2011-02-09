/*
 * Copyright 2001-2002, Haiku Inc. All Rights Reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2010 Clemens Zeidler. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_PROTOCOL_H
#define IMAP_PROTOCOL_H


#include "ServerConnection.h"

#include <vector>

#include <DataIO.h>
#include <ObjectList.h>
#include <OS.h>
#include <String.h>


#define CRLF "\r\n"
#define xEOF    236
const bigtime_t kIMAP4ClientTimeout = 1000000 * 60; // 60 sec


class ConnectionReader {
public:
								ConnectionReader(ServerConnection* connection);

			/*! Try to read line. If no end of line is found at least
			minUnfinishedLine characters are returned. */
			status_t			GetNextLine(BString& line,
									bigtime_t timeout = kIMAP4ClientTimeout,
									uint32 minUnfinishedLine = 128);
			/*! Read data and append it to line till the end of file is
			reached. */
			status_t			FinishLine(BString& line);

			status_t			ReadToFile(int32 size, BPositionIO* out);

private:
			/*! Try to read till the end of line is reached. To do so maximal
			maxNewLength bytes are read from the server if needed. */
			status_t			_GetNextDataBunch(BString& line,
									bigtime_t timeout,
									uint32 maxNewLength = 256);
			bool				_ExtractTillEndOfLine(BString& out);

			ServerConnection*	fServerConnection;

			BString				fStringBuffer;
};


class IMAPCommand;


typedef BObjectList<IMAPCommand> IMAPCommandList;


class IMAPProtocol {
public:
								IMAPProtocol();
			/*! Use the server connection from another protocol. */
								IMAPProtocol(IMAPProtocol& connection);
								~IMAPProtocol();

			/*! Indicates that the current action should be interrupted because
			we are going to be deleted. */
			void				SetStopNow();
			bool				StopNow();

			status_t			Connect(const char* server,
									const char* username, const char* password,
									bool useSSL = true, int32 port = -1);
			status_t			Disconnect();

			ConnectionReader&	GetConnectionReader();
			status_t			SendRawCommand(const char* command);
			int32				SendRawData(const char* buffer, uint32 nBytes);

			status_t			AddAfterQuakeCommand(IMAPCommand* command);

	const	BString&			CommandError() { return fCommandError; }
protected:
			/*! Install a temporary handler at first position in the handler
			list. */
			status_t			ProcessCommand(IMAPCommand* command,
									bigtime_t timeout = kIMAP4ClientTimeout);
			status_t			ProcessCommand(const char* command,
									bigtime_t timeout = kIMAP4ClientTimeout);
			status_t			SendCommand(const char* command,
									int32 commandId);
			status_t			HandleResponse(int32 commandId,
									bigtime_t timeout = kIMAP4ClientTimeout);
			void				ProcessAfterQuacks(bigtime_t timeout);
			int32				NextCommandId();

			ServerConnection*	fServerConnection;
			ServerConnection	fOwnServerConnection;
			ConnectionReader	fConnectionReader;

			IMAPCommandList		fHandlerList;
			IMAPCommandList		fAfterQuackCommands;

private:
			/*! Same as ProccessCommand but AfterShockCommands are not send. */
			status_t			_ProcessCommandWithoutAfterQuake(
									IMAPCommand* command, bigtime_t timeout);
			status_t			_ProcessCommandWithoutAfterQuake(
									const char* command,
									bigtime_t timeout = kIMAP4ClientTimeout);

			int32				fCommandId;
			std::vector<int32>	fOngoingCommands;

			BString				fCommandError;

			vint32				fStopNow;
};


#endif // IMAP_PROTOCOL_H
