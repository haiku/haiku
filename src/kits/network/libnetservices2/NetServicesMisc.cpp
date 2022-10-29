/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include <NetServicesDefs.h>


namespace BPrivate {

namespace Network {


// #pragma mark -- BUnsupportedProtocol


BUnsupportedProtocol::BUnsupportedProtocol(
	const char* origin, BUrl url, BStringList supportedProtocols)
	:
	BError(origin),
	fUrl(std::move(url)),
	fSupportedProtocols(std::move(supportedProtocols))
{
}


BUnsupportedProtocol::BUnsupportedProtocol(BString origin, BUrl url, BStringList supportedProtocols)
	:
	BError(std::move(origin)),
	fUrl(std::move(url)),
	fSupportedProtocols(std::move(supportedProtocols))
{
}


const char*
BUnsupportedProtocol::Message() const noexcept
{
	return "Unsupported Protocol";
}


const BUrl&
BUnsupportedProtocol::Url() const
{
	return fUrl;
}


const BStringList&
BUnsupportedProtocol::SupportedProtocols() const
{
	return fSupportedProtocols;
}


// #pragma mark -- BInvalidUrl


BInvalidUrl::BInvalidUrl(const char* origin, BUrl url)
	:
	BError(origin),
	fUrl(std::move(url))
{
}


BInvalidUrl::BInvalidUrl(BString origin, BUrl url)
	:
	BError(std::move(origin)),
	fUrl(std::move(origin))
{
}


const char*
BInvalidUrl::Message() const noexcept
{
	return "Invalid URL";
}


const BUrl&
BInvalidUrl::Url() const
{
	return fUrl;
}


// #pragma mark -- BNetworkRequestError


BNetworkRequestError::BNetworkRequestError(
	const char* origin, ErrorType type, status_t errorCode, const BString& customMessage)
	:
	BError(origin),
	fErrorType(type),
	fErrorCode(errorCode),
	fCustomMessage(customMessage)
{
}


BNetworkRequestError::BNetworkRequestError(
	const char* origin, ErrorType type, const BString& customMessage)
	:
	BError(origin),
	fErrorType(type),
	fCustomMessage(customMessage)
{
}


const char*
BNetworkRequestError::Message() const noexcept
{
	switch (fErrorType) {
		case HostnameError:
			return "Cannot resolving hostname";
		case NetworkError:
			return "Network error during operation";
		case ProtocolError:
			return "Protocol error";
		case SystemError:
			return "System error";
		case Canceled:
			return "Network request was canceled";
	}
	// Unreachable
	return "Network request error";
}


BString
BNetworkRequestError::DebugMessage() const
{
	BString debugMessage;
	debugMessage << "[" << Origin() << "] " << Message();
	if (fErrorCode != B_OK) {
		debugMessage << "\n\tUnderlying System Error: " << fErrorCode << " ("
					 << strerror(fErrorCode) << ")";
	}
	if (fCustomMessage.Length() > 0) {
		debugMessage << "\n\tAdditional Info: " << fCustomMessage;
	}
	return debugMessage;
}


BNetworkRequestError::ErrorType
BNetworkRequestError::Type() const noexcept
{
	return fErrorType;
}


status_t
BNetworkRequestError::ErrorCode() const noexcept
{
	return fErrorCode;
}


const char*
BNetworkRequestError::CustomMessage() const noexcept
{
	return fCustomMessage.String();
}


// #pragma mark -- Public functions


static const char* kBase64Symbols
	= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";


BString
encode_to_base64(const BString& string)
{
	BString result;
	BString tmpString = string;

	while (tmpString.Length()) {
		char in[3] = {0, 0, 0};
		char out[4] = {0, 0, 0, 0};
		int8 remaining = tmpString.Length();

		tmpString.MoveInto(in, 0, 3);

		out[0] = (in[0] & 0xFC) >> 2;
		out[1] = ((in[0] & 0x03) << 4) | ((in[1] & 0xF0) >> 4);
		out[2] = ((in[1] & 0x0F) << 2) | ((in[2] & 0xC0) >> 6);
		out[3] = in[2] & 0x3F;

		for (int i = 0; i < 4; i++)
			out[i] = kBase64Symbols[(int) out[i]];

		//  Add padding if the input length is not a multiple
		// of 3
		switch (remaining) {
			case 1:
				out[2] = '=';
				// Fall through
			case 2:
				out[3] = '=';
				break;
		}

		result.Append(out, 4);
	}

	return result;
}


// #pragma mark -- message constants
namespace UrlEventData {
const char* Id = "url:identifier";
const char* HostName = "url:hostname";
const char* NumBytes = "url:numbytes";
const char* TotalBytes = "url:totalbytes";
const char* Success = "url:success";
const char* DebugType = "url:debugtype";
const char* DebugMessage = "url:debugmessage";
} // namespace UrlEventData


// #pragma mark -- Private functions and data


static int32 gRequestIdentifier = 1;


int32
get_netservices_request_identifier()
{
	return atomic_add(&gRequestIdentifier, 1);
}


} // namespace Network

} // namespace BPrivate
