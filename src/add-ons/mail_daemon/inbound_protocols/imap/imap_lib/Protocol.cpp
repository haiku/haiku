/*
 * Copyright 2010-2016, Haiku Inc. All Rights Reserved.
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
	fHandlerList(5, false),
	fCommandID(0),
	fIsConnected(false)
{
}


Protocol::~Protocol()
{
	delete fSocket;
	delete fBufferedSocket;
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

	if (fCapabilities.IsEmpty()) {
		CapabilityHandler capabilityHandler;
		status = ProcessCommand(capabilityHandler);
		if (status != B_OK)
			return status;

		_ParseCapabilities(capabilityHandler.Capabilities());
	}

	if (Capabilities().Contains("ID")) {
		// Get the server's ID into our log
		class IDCommand : public IMAP::Command, public IMAP::Handler {
		public:
			BString CommandString()
			{
				return "ID NIL";
			}

			bool HandleUntagged(IMAP::Response& response)
			{
				if (response.IsCommand("ID") && response.IsListAt(1)) {
					puts("Server:");
					ArgumentList& list = response.ListAt(1);
					for (int32 i = 0; i < list.CountItems(); i += 2) {
						printf("  %s: %s\n",
							list.ItemAt(i)->ToString().String(),
							list.ItemAt(i + 1)->ToString().String());
					}
					return true;
				}

				return false;
			}
		};
		IDCommand idCommand;
		ProcessCommand(idCommand);
	}
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


bool
Protocol::AddHandler(Handler& handler)
{
	return fHandlerList.AddItem(&handler);
}


void
Protocol::RemoveHandler(Handler& handler)
{
	fHandlerList.RemoveItem(&handler);
}


status_t
Protocol::GetFolders(FolderList& folders, BString& separator)
{
	BStringList allFolders;
	status_t status = _GetAllFolders(allFolders);
	if (status != B_OK)
		return status;

	BStringList subscribedFolders;
	status = GetSubscribedFolders(subscribedFolders, separator);
	if (status != B_OK)
		return status;

	for (int32 i = 0; i < allFolders.CountStrings(); i++) {
		FolderEntry entry;
		entry.folder = allFolders.StringAt(i);
		for (int32 j = 0; j < subscribedFolders.CountStrings(); j++) {
			if (entry.folder == subscribedFolders.StringAt(j)) {
				entry.subscribed = true;
				break;
			}
		}
		folders.push_back(entry);
	}

	// you could be subscribed to a folder which not exist currently, add them:
	for (int32 i = 0; i < subscribedFolders.CountStrings(); i++) {
		bool isInlist = false;
		for (int32 j = 0; j < allFolders.CountStrings(); j++) {
			if (subscribedFolders.StringAt(i) == allFolders.StringAt(j)) {
				isInlist = true;
				break;
			}
		}
		if (isInlist)
			continue;

		FolderEntry entry;
		entry.folder = subscribedFolders.StringAt(i);
		entry.subscribed = true;
		folders.push_back(entry);
	}

	return B_OK;
}


status_t
Protocol::GetSubscribedFolders(BStringList& folders, BString& separator)
{
	ListCommand command(NULL, true);
	status_t status = ProcessCommand(command);
	if (status != B_OK)
		return status;

	folders = command.FolderList();
	separator = command.Separator();
	return status;
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
	if (id > 0) {
		length = snprintf(buffer, sizeof(buffer), "A%.7" B_PRId32 " %s\r\n",
			id, command);
	} else
		length = snprintf(buffer, sizeof(buffer), "%s\r\n", command);

	TRACE("C: %s", buffer);

	ssize_t bytesWritten = fSocket->Write(buffer, length);
	if (bytesWritten < 0)
		return bytesWritten;

	return bytesWritten == length ? B_OK : B_ERROR;
}


ssize_t
Protocol::SendData(const char* buffer, uint32 length)
{
	return fSocket->Write(buffer, length);
}


status_t
Protocol::ProcessCommand(Command& command, bigtime_t timeout)
{
	BString commandString = command.CommandString();
	if (commandString.IsEmpty())
		return B_BAD_VALUE;

	Handler* handler = dynamic_cast<Handler*>(&command);
	if (handler != NULL && !AddHandler(*handler))
		return B_NO_MEMORY;

	int32 commandID = NextCommandID();
	status_t status = SendCommand(commandID, commandString.String());
	if (status == B_OK) {
		fOngoingCommands[commandID] = &command;
		status = HandleResponse(&command, timeout);
	}

	if (handler != NULL)
		RemoveHandler(*handler);

	return status;
}


// #pragma mark - protected


status_t
Protocol::HandleResponse(Command* command, bigtime_t timeout,
	bool disconnectOnTimeout)
{
	status_t commandStatus = B_OK;
	IMAP::ResponseParser parser(*fBufferedSocket);
	if (IMAP::LiteralHandler* literalHandler
			= dynamic_cast<IMAP::LiteralHandler*>(command))
		parser.SetLiteralHandler(literalHandler);

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
				for (int32 i = fHandlerList.CountItems(); i-- > 0;) {
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
		} catch (StreamException& exception) {
			return exception.Status();
		}

		if (fOngoingCommands.size() == 0)
			done = true;
	}

	return commandStatus;
}


int32
Protocol::NextCommandID()
{
	fCommandID++;
	return fCommandID;
}


// #pragma mark - private


status_t
Protocol::_Disconnect()
{
	fOngoingCommands.clear();
	fIsConnected = false;
	delete fBufferedSocket;
	fBufferedSocket = NULL;
	delete fSocket;
	fSocket = NULL;

	return B_OK;
}


status_t
Protocol::_GetAllFolders(BStringList& folders)
{
	ListCommand command(NULL, false);
	status_t status = ProcessCommand(command);
	if (status != B_OK)
		return status;

	folders = command.FolderList();
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
