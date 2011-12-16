/*
 * Copyright 2010-2011, Haiku Inc. All Rights Reserved.
 * Copyright 2010 Clemens Zeidler. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "IMAPProtocol.h"

#include "IMAPHandler.h"
#include "IMAPParser.h"


#define DEBUG_IMAP_PROTOCOL
#ifdef DEBUG_IMAP_PROTOCOL
#	include <stdio.h>
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) ;
#endif


ConnectionReader::ConnectionReader(ServerConnection* connection)
	:
	fServerConnection(connection)
{
}


status_t
ConnectionReader::GetNextLine(BString& line, bigtime_t timeout,
	int32 maxUnfinishedLine)
{
	line.SetTo((const char*)NULL, 0);

	while (true) {
		status_t status = _GetNextDataBunch(line, timeout);
		if (status == B_OK)
			return status;
		if (status == B_NAME_NOT_FOUND) {
			if (maxUnfinishedLine < 0 || line.Length() < maxUnfinishedLine)
				continue;
			else
				return status;
		}
		return status;
	}
	return B_ERROR;
}


status_t
ConnectionReader::FinishLine(BString& line)
{
	while (true) {
		status_t status = _GetNextDataBunch(line, B_INFINITE_TIMEOUT);
		if (status == B_OK)
			return status;
		if (status == B_NAME_NOT_FOUND)
			continue;
		return status;
	}
	return B_ERROR;
}


status_t
ConnectionReader::ReadToFile(int32 size, BPositionIO* out)
{
	const int32 kBunchSize = 1024; // 1Kb
	char buffer[kBunchSize];

	int32 readSize = size - fStringBuffer.Length();
	int32 readed = fStringBuffer.Length();
	if (readSize < 0) {
		readed = size;
	}
	out->Write(fStringBuffer.String(), readed);
	fStringBuffer.Remove(0, readed);

	while (readSize > 0) {
		int32 bunchSize = readSize < kBunchSize ? readSize : kBunchSize;
		int nReaded = fServerConnection->Read(buffer, bunchSize);
		if (nReaded < 0)
			return B_ERROR;
		readSize -= nReaded;
		out->Write(buffer, nReaded);
	}
	return B_OK;
}


status_t
ConnectionReader::_GetNextDataBunch(BString& line, bigtime_t timeout,
	uint32 maxNewLength)
{
	if (_ExtractTillEndOfLine(line))
		return B_OK;

	char buffer[maxNewLength];

	if (timeout != B_INFINITE_TIMEOUT) {
		status_t status = fServerConnection->WaitForData(timeout);
		if (status != B_OK)
			return status;
	}

	int nReaded = fServerConnection->Read(buffer, maxNewLength);
	if (nReaded <= 0)
		return B_ERROR;

	fStringBuffer.SetTo(buffer, nReaded);
	if (_ExtractTillEndOfLine(line))
		return B_OK;
	return B_NAME_NOT_FOUND;
}


bool
ConnectionReader::_ExtractTillEndOfLine(BString& out)
{
	int32 endPos = fStringBuffer.FindFirst('\n');
	if (endPos == B_ERROR) {
		endPos = fStringBuffer.FindFirst(xEOF);
		if (endPos == B_ERROR) {
			out += fStringBuffer;
			fStringBuffer.SetTo((const char*)NULL, 0);
			return false;
		}
	}
	out.Append(fStringBuffer, endPos + 1);
	fStringBuffer.Remove(0, endPos + 1);

	return true;
}


// #pragma mark -


IMAPProtocol::IMAPProtocol()
	:
	fServerConnection(&fOwnServerConnection),
	fConnectionReader(fServerConnection),
	fCommandID(0),
	fStopNow(0),
	fIsConnected(false)
{
}


IMAPProtocol::IMAPProtocol(IMAPProtocol& connection)
	:
	fServerConnection(connection.fServerConnection),
	fConnectionReader(fServerConnection),
	fCommandID(0),
	fStopNow(0),
	fIsConnected(false)
{
}


IMAPProtocol::~IMAPProtocol()
{
	for (int32 i = 0; i < fAfterQuackCommands.CountItems(); i++)
		delete fAfterQuackCommands.ItemAt(i);
}



void
IMAPProtocol::SetStopNow()
{
	atomic_set(&fStopNow, 1);
}


bool
IMAPProtocol::StopNow()
{
	return (atomic_get(&fStopNow) != 0);
}


status_t
IMAPProtocol::Connect(const char* server, const char* username,
	const char* password, bool useSSL, int32 port)
{
	TRACE("Connect\n");
	status_t status = B_ERROR;
	if (useSSL) {
		if (port >= 0)
			status = fServerConnection->ConnectSSL(server, port);
		else
			status = fServerConnection->ConnectSSL(server);
	} else {
		if (port >= 0)
			status = fServerConnection->ConnectSocket(server, port);
		else
			status = fServerConnection->ConnectSocket(server);
	}
	if (status != B_OK)
		return status;

	TRACE("Login\n");

	fIsConnected = true;

	BString command = "LOGIN ";
	command << "\"" << username << "\" ";
	command << "\"" << password << "\"";
	status = ProcessCommand(command);
	if (status != B_OK) {
		_Disconnect();
		return status;
	}

	return B_OK;
}


status_t
IMAPProtocol::Disconnect()
{
	ProcessCommand("LOGOUT");
	return _Disconnect();
}


bool
IMAPProtocol::IsConnected()
{
	return fIsConnected;
}


ConnectionReader&
IMAPProtocol::GetConnectionReader()
{
	return fConnectionReader;
}


status_t
IMAPProtocol::SendRawCommand(const char* command)
{
	static char cmd[256];
	::sprintf(cmd, "%s"CRLF, command);
	int32 commandLength = strlen(cmd);

	if (fServerConnection->Write(cmd, commandLength) != commandLength)
		return B_ERROR;
	return B_OK;
}


int32
IMAPProtocol::SendRawData(const char* buffer, uint32 nBytes)
{
	return fServerConnection->Write(buffer, nBytes);
}


status_t
IMAPProtocol::AddAfterQuakeCommand(IMAPCommand* command)
{
	return fAfterQuackCommands.AddItem(command);
}


status_t
IMAPProtocol::ProcessCommand(IMAPCommand* command, bigtime_t timeout)
{
	status_t status = _ProcessCommandWithoutAfterQuake(command, timeout);

	ProcessAfterQuacks(timeout);

	return status;
}


status_t
IMAPProtocol::ProcessCommand(const char* command, bigtime_t timeout)
{
	status_t status = _ProcessCommandWithoutAfterQuake(command, timeout);

	ProcessAfterQuacks(timeout);
	return status;
}


status_t
IMAPProtocol::SendCommand(const char* command, int32 commandID)
{
	if (strlen(command) + 10 > 256)
		return B_NO_MEMORY;

	static char cmd[256];
	::sprintf(cmd, "A%.7ld %s"CRLF, commandID, command);

	TRACE("_SendCommand: %s\n", cmd);
	int commandLength = strlen(cmd);
	if (fServerConnection->Write(cmd, commandLength) != commandLength) {
		// we might lost the connection, clear the connection state
		_Disconnect();
		return B_ERROR;
	}

	fOngoingCommands.push_back(commandID);
	return B_OK;
}


status_t
IMAPProtocol::HandleResponse(int32 commandID, bigtime_t timeout,
	bool disconnectOnTimeout)
{
	status_t commandStatus = B_ERROR;

	bool done = false;
	while (done != true) {
		BString line;
		status_t status = fConnectionReader.GetNextLine(line, timeout);
		if (status != B_OK) {
			// we might lost the connection, clear the connection state
			if (status != B_TIMED_OUT) {
				TRACE("S:read error %s", line.String());
				_Disconnect();
			} else if (disconnectOnTimeout) {
				_Disconnect();
			}
			return status;
		}
		//TRACE("S: %s", line.String());

		bool handled = false;
		for (int i = 0; i < fHandlerList.CountItems(); i++) {
			if (fHandlerList.ItemAt(i)->Handle(line) == true) {
				handled = true;
				break;
			}
		}
		if (handled == true)
			continue;

		for (std::vector<int32>::iterator it = fOngoingCommands.begin();
			it != fOngoingCommands.end(); it++) {
			static char idString[8];
			::sprintf(idString, "A%.7ld", *it);
			if (line.FindFirst(idString) >= 0) {
				if (*it == commandID) {
					BString result = IMAPParser::ExtractElementAfter(line,
						idString);
					if (result == "OK")
						commandStatus = B_OK;
					else {
						fCommandError = IMAPParser::ExtractStringAfter(line,
							idString);
						TRACE("Command Error %s\n", fCommandError.String());
					}
				}
				fOngoingCommands.erase(it);
				break;
			}
		}
		if (fOngoingCommands.size() == 0)
			done = true;

		TRACE("Unhandled S: %s", line.String());
	}

	return commandStatus;
}


void
IMAPProtocol::ProcessAfterQuacks(bigtime_t timeout)
{
	while (fAfterQuackCommands.CountItems() != 0) {
		IMAPCommand* currentCommand = fAfterQuackCommands.RemoveItemAt(0);
		_ProcessCommandWithoutAfterQuake(currentCommand, timeout);
		delete currentCommand;
	}
}


int32
IMAPProtocol::NextCommandID()
{
	fCommandID++;
	return fCommandID;
}


status_t
IMAPProtocol::_ProcessCommandWithoutAfterQuake(IMAPCommand* command,
	bigtime_t timeout)
{
	BString cmd = command->Command();
	if (cmd == "")
		return B_BAD_VALUE;
	if (!fHandlerList.AddItem(command, 0))
		return B_NO_MEMORY;

	status_t status = _ProcessCommandWithoutAfterQuake(cmd, timeout);

	fHandlerList.RemoveItem(command);

	return status;
}


status_t
IMAPProtocol::_ProcessCommandWithoutAfterQuake(const char* command,
	bigtime_t timeout)
{
	int32 commandID = NextCommandID();
	status_t status = SendCommand(command, commandID);
	if (status != B_OK)
		return status;

	return HandleResponse(commandID, timeout);
}


status_t
IMAPProtocol::_Disconnect()
{
	fOngoingCommands.clear();
	fIsConnected = false;
	return fOwnServerConnection.Disconnect();
}
