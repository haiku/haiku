/*
 * Copyright 2010-2011, Haiku Inc. All Rights Reserved.
 * Copyright 2010 Clemens Zeidler. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "Protocol.h"

#include "Commands.h"


#define DEBUG_IMAP_PROTOCOL
#ifdef DEBUG_IMAP_PROTOCOL
#	include <stdio.h>
#	define TRACE(...) printf(__VA_ARGS__)
#else
#	define TRACE(...) ;
#endif


namespace IMAP {


Protocol::Protocol()
	:
	fSocket(NULL),
	fBufferedSocket(NULL),
	fCommandID(0),
	fStopNow(0),
	fIsConnected(false)
{
}


Protocol::~Protocol()
{
	for (int32 i = 0; i < fAfterQuackCommands.CountItems(); i++)
		delete fAfterQuackCommands.ItemAt(i);

	delete fSocket;
	delete fBufferedSocket;
}



void
Protocol::SetStopNow()
{
	atomic_set(&fStopNow, 1);
}


bool
Protocol::StopNow()
{
	return (atomic_get(&fStopNow) != 0);
}


status_t
Protocol::Connect(const BNetworkAddress& address, const char* username,
	const char* password, bool useSSL)
{
	TRACE("Connect\n");
	if (useSSL)
		fSocket = new(std::nothrow) BSecureSocket(address);
	else
		fSocket = new(std::nothrow) BSocket(address);

	if (fSocket == NULL)
		return B_NO_MEMORY;

	status_t status = fSocket->InitCheck();
	if (status != B_OK)
		return status;

	fBufferedSocket = new(std::nothrow) BBufferedDataIO(*fSocket, 32768, false,
		true);
	if (fBufferedSocket == NULL)
		return B_NO_MEMORY;

	TRACE("Login\n");

	fIsConnected = true;

	LoginCommand login(username, password);
	status = ProcessCommand(login);
	if (status != B_OK) {
		_Disconnect();
		return status;
	}

	_ParseCapabilities(login.Capabilities());
	return B_OK;
}


status_t
Protocol::Disconnect()
{
	if (IsConnected()) {
		RawCommand command("LOGOUT");
		ProcessCommand(command);
	}
	return _Disconnect();
}


bool
Protocol::IsConnected()
{
	return fIsConnected;
}


status_t
Protocol::GetFolders(FolderList& folders)
{
	StringList allFolders;
	status_t status = _GetAllFolders(allFolders);
	if (status != B_OK)
		return status;

	StringList subscribedFolders;
	status = _GetSubscribedFolders(subscribedFolders);
	if (status != B_OK)
		return status;

	for (unsigned int i = 0; i < allFolders.size(); i++) {
		FolderInfo info;
		info.folder = allFolders[i];
		for (unsigned int a = 0; a < subscribedFolders.size(); a++) {
			if (allFolders[i] == subscribedFolders[a]
				|| allFolders[i].ICompare("INBOX") == 0) {
				info.subscribed = true;
				break;
			}
		}
		folders.push_back(info);
	}

	// you could be subscribed to a folder which not exist currently, add them:
	for (unsigned int a = 0; a < subscribedFolders.size(); a++) {
		bool isInlist = false;
		for (unsigned int i = 0; i < allFolders.size(); i++) {
			if (subscribedFolders[a] == allFolders[i]) {
				isInlist = true;
				break;
			}
		}
		if (isInlist)
			continue;

		FolderInfo info;
		info.folder = subscribedFolders[a];
		info.subscribed = true;
		folders.push_back(info);
	}

	return B_OK;
}


status_t
Protocol::SubscribeFolder(const char* folder)
{
	SubscribeCommand command(folder);
	return ProcessCommand(command);
}


status_t
Protocol::UnsubscribeFolder(const char* folder)
{
	UnsubscribeCommand command(folder);
	return ProcessCommand(command);
}


status_t
Protocol::SelectMailbox(const char* mailbox)
{
	TRACE("SELECT %s\n", mailbox);
	SelectCommand command(mailbox);
	status_t status = ProcessCommand(command);
	if (status != B_OK)
		return status;

	if (fCapabilities.IsEmpty()) {
		CapabilityHandler capabilityHandler;
		status = ProcessCommand(capabilityHandler);
		if (status != B_OK)
			return status;

		_ParseCapabilities(capabilityHandler.Capabilities());
	}

	fMailbox = mailbox;
	return B_OK;
}


status_t
Protocol::GetQuota(uint64& used, uint64& total)
{
	if (!Capabilities().Contains("QUOTA"))
		return B_ERROR;

	GetQuotaCommand quotaCommand;
	status_t status = ProcessCommand(quotaCommand);
	if (status != B_OK)
		return status;

	used = quotaCommand.UsedStorage();
	total = quotaCommand.TotalStorage();
	return B_OK;
}


status_t
Protocol::SendCommand(const char* command)
{
	return SendCommand(0, command);
}


status_t
Protocol::SendCommand(int32 id, const char* command)
{
	char buffer[2048];
	int32 length;
	if (id > 0)
		length = snprintf(buffer, sizeof(buffer), "A%.7ld %s\r\n", id, command);
	else
		length = snprintf(buffer, sizeof(buffer), "%s\r\n", command);

	ssize_t bytesWritten = fSocket->Write(buffer, length);
	if (bytesWritten < 0)
		return bytesWritten;

	return bytesWritten == length ? B_OK : B_ERROR;
}


int32
Protocol::SendData(const char* buffer, uint32 length)
{
	return fSocket->Write(buffer, length);
}


status_t
Protocol::AddAfterQuakeCommand(Command* command)
{
	return fAfterQuackCommands.AddItem(command);
}


status_t
Protocol::ProcessCommand(Command& command, bigtime_t timeout)
{
	status_t status = _ProcessCommandWithoutAfterQuake(command, timeout);

	ProcessAfterQuacks(timeout);

	return status;
}


status_t
Protocol::HandleResponse(bigtime_t timeout, bool disconnectOnTimeout)
{
	status_t commandStatus = B_OK;
	IMAP::ResponseParser parser(*fBufferedSocket);
	IMAP::Response response;

	bool done = false;
	while (!done) {
		try {
			status_t status = parser.NextResponse(response, timeout);
			if (status != B_OK) {
				// we might have lost the connection, clear the connection state
				if (status != B_TIMED_OUT || disconnectOnTimeout)
					_Disconnect();

				return status;
			}

			if (response.IsUntagged() || response.IsContinuation()) {
				bool handled = false;
				for (int i = 0; i < fHandlerList.CountItems(); i++) {
					if (fHandlerList.ItemAt(i)->HandleUntagged(response)) {
						handled = true;
						break;
					}
				}
				if (!handled)
					printf("Unhandled S: %s\n", response.ToString().String());
			} else {
				CommandIDMap::iterator found
					= fOngoingCommands.find(response.Tag());
				if (found != fOngoingCommands.end()) {
					status_t status = found->second->HandleTagged(response);
					if (status != B_OK)
						commandStatus = status;

					fOngoingCommands.erase(found);
				} else
					printf("Unknown tag S: %s\n", response.ToString().String());
			}
		} catch (ParseException& exception) {
			printf("Error during parsing: %s\n", exception.Message());
		}

		if (fOngoingCommands.size() == 0)
			done = true;
	}

	return commandStatus;
}


void
Protocol::ProcessAfterQuacks(bigtime_t timeout)
{
	while (fAfterQuackCommands.CountItems() != 0) {
		Command* currentCommand = fAfterQuackCommands.RemoveItemAt(0);
		_ProcessCommandWithoutAfterQuake(*currentCommand, timeout);
		delete currentCommand;
	}
}


int32
Protocol::NextCommandID()
{
	fCommandID++;
	return fCommandID;
}


status_t
Protocol::_ProcessCommandWithoutAfterQuake(Command& command, bigtime_t timeout)
{
	BString commandString = command.CommandString();
	if (commandString.IsEmpty())
		return B_BAD_VALUE;

	Handler* handler = dynamic_cast<Handler*>(&command);
	if (handler != NULL && !fHandlerList.AddItem(handler, 0))
		return B_NO_MEMORY;

	int32 commandID = NextCommandID();
	status_t status = SendCommand(commandID, commandString.String());
	if (status == B_OK) {
		fOngoingCommands[commandID] = &command;
		status = HandleResponse(timeout);
	}

	if (handler != NULL)
		fHandlerList.RemoveItem(handler);

	return status;
}


status_t
Protocol::_Disconnect()
{
	fOngoingCommands.clear();
	fIsConnected = false;
	delete fSocket;
	fSocket = NULL;
	return B_OK;
}


status_t
Protocol::_GetAllFolders(StringList& folders)
{
	ListCommand listCommand;
	status_t status = ProcessCommand(listCommand);
	if (status != B_OK)
		return status;

	folders = listCommand.FolderList();
	return status;
}


status_t
Protocol::_GetSubscribedFolders(StringList& folders)
{
	ListSubscribedCommand listSubscribedCommand;
	status_t status = ProcessCommand(listSubscribedCommand);
	if (status != B_OK)
		return status;

	folders = listSubscribedCommand.FolderList();
	return status;
}


void
Protocol::_ParseCapabilities(const ArgumentList& arguments)
{
	fCapabilities.MakeEmpty();

	for (int32 i = 0; i < arguments.CountItems(); i++) {
		if (StringArgument* argument
				= dynamic_cast<StringArgument*>(arguments.ItemAt(i)))
			fCapabilities.AddItem(new StringArgument(*argument));
	}

	TRACE("capabilities: %s\n", fCapabilities.ToString().String());
}


}	// namespace IMAP
