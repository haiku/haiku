/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include "HttpDebugLogger.h"

#include <iostream>

#include <ErrorsExt.h>
#include <HttpSession.h>
#include <NetServicesDefs.h>

using namespace BPrivate::Network;


HttpDebugLogger::HttpDebugLogger()
	:
	BLooper("HttpDebugLogger")
{
}


void
HttpDebugLogger::SetConsoleLogging(bool enabled)
{
	fConsoleLogging = enabled;
}


void
HttpDebugLogger::SetFileLogging(const char* path)
{
	if (auto status = fLogFile.SetTo(path, B_WRITE_ONLY | B_CREATE_FILE | B_OPEN_AT_END);
		status != B_OK)
		throw BSystemError("BFile::SetTo()", status);
}


void
HttpDebugLogger::MessageReceived(BMessage* message)
{
	BString output;

	if (!message->HasInt32(UrlEventData::Id))
		return BLooper::MessageReceived(message);
	int32 id = message->FindInt32(UrlEventData::Id);
	output << "[" << id << "] ";

	switch (message->what) {
		case UrlEvent::HostNameResolved:
		{
			BString hostname;
			message->FindString(UrlEventData::HostName, &hostname);
			output << "<HostNameResolved> " << hostname;
			break;
		}
		case UrlEvent::ConnectionOpened:
			output << "<ConnectionOpened>";
			break;
		case UrlEvent::UploadProgress:
		{
			off_t numBytes = message->GetInt64(UrlEventData::NumBytes, -1);
			off_t totalBytes = message->GetInt64(UrlEventData::TotalBytes, -1);
			output << "<UploadProgress> bytes uploaded " << numBytes;
			if (totalBytes == -1)
				output << " (total unknown)";
			else
				output << " (" << totalBytes << " total)";
			break;
		}
		case UrlEvent::ResponseStarted:
		{
			output << "<ResponseStarted>";
			break;
		}
		case UrlEvent::HttpRedirect:
		{
			BString redirectUrl;
			message->FindString(UrlEventData::HttpRedirectUrl, &redirectUrl);
			output << "<HttpRedirect> to: " << redirectUrl;
			break;
		}
		case UrlEvent::HttpStatus:
		{
			int16 status = message->FindInt16(UrlEventData::HttpStatusCode);
			output << "<HttpStatus> code: " << status;
			break;
		}
		case UrlEvent::HttpFields:
		{
			output << "<HttpFields> All fields parsed";
			break;
		}
		case UrlEvent::DownloadProgress:
		{
			off_t numBytes = message->GetInt64(UrlEventData::NumBytes, -1);
			off_t totalBytes = message->GetInt64(UrlEventData::TotalBytes, -1);
			output << "<DownloadProgress> bytes downloaded " << numBytes;
			if (totalBytes == -1)
				output << " (total unknown)";
			else
				output << " (" << totalBytes << " total)";
			break;
		}
		case UrlEvent::BytesWritten:
		{
			off_t numBytes = message->GetInt64(UrlEventData::NumBytes, -1);
			output << "<BytesWritten> bytes written to output: " << numBytes;
			break;
		}
		case UrlEvent::RequestCompleted:
		{
			bool success = message->GetBool(UrlEventData::Success, false);
			output << "<RequestCompleted> success: ";
			if (success)
				output << "true";
			else
				output << "false";
			break;
		}
		case UrlEvent::DebugMessage:
		{
			uint32 debugType = message->GetUInt32(UrlEventData::DebugType, 0);
			BString debugMessage;
			message->FindString(UrlEventData::DebugMessage, &debugMessage);
			output << "<DebugMessage> ";
			switch (debugType) {
				case UrlEventData::DebugInfo:
					output << "INFO: ";
					break;
				case UrlEventData::DebugWarning:
					output << "WARNING: ";
					break;
				case UrlEventData::DebugError:
					output << "ERROR: ";
					break;
				default:
					output << "UNKNOWN: ";
					break;
			}
			output << debugMessage;
			break;
		}
		default:
			return BLooper::MessageReceived(message);
	}

	if (fConsoleLogging)
		std::cout << output.String() << std::endl;

	if (fLogFile.InitCheck() == B_OK) {
		output += '\n';
		if (auto status = fLogFile.WriteExactly(output.String(), output.Length()); status != B_OK)
			throw BSystemError("BFile::WriteExactly()", status);
		if (auto status = fLogFile.Flush(); status != B_OK)
			throw BSystemError("BFile::Flush()", status);
	}
}
