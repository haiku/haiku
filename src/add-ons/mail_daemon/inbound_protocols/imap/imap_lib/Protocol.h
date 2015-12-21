/*
 * Copyright 2001-2015, Haiku Inc. All Rights Reserved.
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


const bigtime_t kIMAP4ClientTimeout = 1000000 * 60;
	// 60 seconds


namespace IMAP {


class Command;
class Handler;


typedef BObjectList<Handler> HandlerList;
typedef std::map<int32, Command*> CommandIDMap;


class FolderEntry {
public:
	FolderEntry()
		:
		subscribed(false)
	{
	}

	BString	folder;
	bool	subscribed;
};
typedef std::vector<FolderEntry> FolderList;


class Protocol {
public:
								Protocol();
								~Protocol();

			status_t			Connect(const BNetworkAddress& address,
									const char* username, const char* password,
									bool useSSL = true);
			status_t			Disconnect();
			bool				IsConnected();

			bool				AddHandler(Handler& handler);
			void				RemoveHandler(Handler& handler);

			// Some convenience methods
			status_t			GetFolders(FolderList& folders,
									BString& separator);
			status_t			GetSubscribedFolders(BStringList& folders,
									BString& separator);
			status_t			SubscribeFolder(const char* folder);
			status_t			UnsubscribeFolder(const char* folder);
			status_t			GetQuota(uint64& used, uint64& total);

			status_t			SendCommand(const char* command);
			status_t			SendCommand(int32 id, const char* command);
			ssize_t				SendData(const char* buffer, uint32 length);

			status_t			ProcessCommand(Command& command,
									bigtime_t timeout = kIMAP4ClientTimeout);

			ArgumentList&		Capabilities() { return fCapabilities; }
			const ArgumentList&	Capabilities() const { return fCapabilities; }

			const BString&		CommandError() { return fCommandError; }

protected:
			status_t			HandleResponse(Command* command,
									bigtime_t timeout = kIMAP4ClientTimeout,
									bool disconnectOnTimeout = true);
			int32				NextCommandID();

private:
			status_t			_Disconnect();
			status_t			_GetAllFolders(BStringList& folders);
			void				_ParseCapabilities(
									const ArgumentList& arguments);

protected:
			BSocket* 			fSocket;
			BBufferedDataIO*	fBufferedSocket;

			HandlerList			fHandlerList;
			ArgumentList		fCapabilities;

private:
			int32				fCommandID;
			CommandIDMap		fOngoingCommands;
			BString				fCommandError;

			bool				fIsConnected;
};


}	// namespace IMAP


#endif // PROTOCOL_H
