/*
 * Copyright 2010-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <HttpRequest.h>

#include <arpa/inet.h>
#include <stdio.h>

#include <cstdlib>
#include <deque>
#include <new>

#include <AutoDeleter.h>
#include <Certificate.h>
#include <Debug.h>
#include <DynamicBuffer.h>
#include <File.h>
#include <ProxySecureSocket.h>
#include <Socket.h>
#include <SecureSocket.h>
#include <StackOrHeapArray.h>
#include <ZlibCompressionAlgorithm.h>


static const int32 kHttpBufferSize = 4096;


namespace BPrivate {

	class CheckedSecureSocket: public BSecureSocket
	{
		public:
			CheckedSecureSocket(BHttpRequest* request);

			bool			CertificateVerificationFailed(BCertificate& certificate,
					const char* message);

		private:
			BHttpRequest*	fRequest;
	};


	CheckedSecureSocket::CheckedSecureSocket(BHttpRequest* request)
		:
		BSecureSocket(),
		fRequest(request)
	{
	}


	bool
	CheckedSecureSocket::CertificateVerificationFailed(BCertificate& certificate,
		const char* message)
	{
		return fRequest->_CertificateVerificationFailed(certificate, message);
	}


	class CheckedProxySecureSocket: public BProxySecureSocket
	{
		public:
			CheckedProxySecureSocket(const BNetworkAddress& proxy, BHttpRequest* request);

			bool			CertificateVerificationFailed(BCertificate& certificate,
					const char* message);

		private:
			BHttpRequest*	fRequest;
	};


	CheckedProxySecureSocket::CheckedProxySecureSocket(const BNetworkAddress& proxy,
		BHttpRequest* request)
		:
		BProxySecureSocket(proxy),
		fRequest(request)
	{
	}


	bool
	CheckedProxySecureSocket::CertificateVerificationFailed(BCertificate& certificate,
		const char* message)
	{
		return fRequest->_CertificateVerificationFailed(certificate, message);
	}
};


BHttpRequest::BHttpRequest(const BUrl& url, bool ssl, const char* protocolName,
	BUrlProtocolListener* listener, BUrlContext* context)
	:
	BNetworkRequest(url, listener, context, "BUrlProtocol.HTTP", protocolName),
	fSSL(ssl),
	fRequestMethod(B_HTTP_GET),
	fHttpVersion(B_HTTP_11),
	fResult(url),
	fRequestStatus(kRequestInitialState),
	fOptHeaders(NULL),
	fOptPostFields(NULL),
	fOptInputData(NULL),
	fOptInputDataSize(-1),
	fOptRangeStart(-1),
	fOptRangeEnd(-1),
	fOptFollowLocation(true)
{
	_ResetOptions();
	fSocket = NULL;
}


BHttpRequest::BHttpRequest(const BHttpRequest& other)
	:
	BNetworkRequest(other.Url(), other.fListener, other.fContext,
		"BUrlProtocol.HTTP", other.fSSL ? "HTTPS" : "HTTP"),
	fSSL(other.fSSL),
	fRequestMethod(other.fRequestMethod),
	fHttpVersion(other.fHttpVersion),
	fResult(other.fUrl),
	fRequestStatus(kRequestInitialState),
	fOptHeaders(NULL),
	fOptPostFields(NULL),
	fOptInputData(NULL),
	fOptInputDataSize(-1),
	fOptRangeStart(other.fOptRangeStart),
	fOptRangeEnd(other.fOptRangeEnd),
	fOptFollowLocation(other.fOptFollowLocation)
{
	_ResetOptions();
		// FIXME some options may be copied from other instead.
	fSocket = NULL;
}


BHttpRequest::~BHttpRequest()
{
	Stop();

	delete fSocket;

	delete fOptInputData;
	delete fOptHeaders;
	delete fOptPostFields;
}


void
BHttpRequest::SetMethod(const char* const method)
{
	fRequestMethod = method;
}


void
BHttpRequest::SetFollowLocation(bool follow)
{
	fOptFollowLocation = follow;
}


void
BHttpRequest::SetMaxRedirections(int8 redirections)
{
	fOptMaxRedirs = redirections;
}


void
BHttpRequest::SetReferrer(const BString& referrer)
{
	fOptReferer = referrer;
}


void
BHttpRequest::SetUserAgent(const BString& agent)
{
	fOptUserAgent = agent;
}


void
BHttpRequest::SetDiscardData(bool discard)
{
	fOptDiscardData = discard;
}


void
BHttpRequest::SetDisableListener(bool disable)
{
	fOptDisableListener = disable;
}


void
BHttpRequest::SetAutoReferrer(bool enable)
{
	fOptAutoReferer = enable;
}


void
BHttpRequest::SetHeaders(const BHttpHeaders& headers)
{
	AdoptHeaders(new(std::nothrow) BHttpHeaders(headers));
}


void
BHttpRequest::AdoptHeaders(BHttpHeaders* const headers)
{
	delete fOptHeaders;
	fOptHeaders = headers;
}


void
BHttpRequest::SetPostFields(const BHttpForm& fields)
{
	AdoptPostFields(new(std::nothrow) BHttpForm(fields));
}


void
BHttpRequest::AdoptPostFields(BHttpForm* const fields)
{
	delete fOptPostFields;
	fOptPostFields = fields;

	if (fOptPostFields != NULL)
		fRequestMethod = B_HTTP_POST;
}


void
BHttpRequest::AdoptInputData(BDataIO* const data, const ssize_t size)
{
	delete fOptInputData;
	fOptInputData = data;
	fOptInputDataSize = size;
}


void
BHttpRequest::SetUserName(const BString& name)
{
	fOptUsername = name;
}


void
BHttpRequest::SetPassword(const BString& password)
{
	fOptPassword = password;
}


/*static*/ bool
BHttpRequest::IsInformationalStatusCode(int16 code)
{
	return (code >= B_HTTP_STATUS__INFORMATIONAL_BASE)
		&& (code <  B_HTTP_STATUS__INFORMATIONAL_END);
}


/*static*/ bool
BHttpRequest::IsSuccessStatusCode(int16 code)
{
	return (code >= B_HTTP_STATUS__SUCCESS_BASE)
		&& (code <  B_HTTP_STATUS__SUCCESS_END);
}


/*static*/ bool
BHttpRequest::IsRedirectionStatusCode(int16 code)
{
	return (code >= B_HTTP_STATUS__REDIRECTION_BASE)
		&& (code <  B_HTTP_STATUS__REDIRECTION_END);
}


/*static*/ bool
BHttpRequest::IsClientErrorStatusCode(int16 code)
{
	return (code >= B_HTTP_STATUS__CLIENT_ERROR_BASE)
		&& (code <  B_HTTP_STATUS__CLIENT_ERROR_END);
}


/*static*/ bool
BHttpRequest::IsServerErrorStatusCode(int16 code)
{
	return (code >= B_HTTP_STATUS__SERVER_ERROR_BASE)
		&& (code <  B_HTTP_STATUS__SERVER_ERROR_END);
}


/*static*/ int16
BHttpRequest::StatusCodeClass(int16 code)
{
	if (BHttpRequest::IsInformationalStatusCode(code))
		return B_HTTP_STATUS_CLASS_INFORMATIONAL;
	else if (BHttpRequest::IsSuccessStatusCode(code))
		return B_HTTP_STATUS_CLASS_SUCCESS;
	else if (BHttpRequest::IsRedirectionStatusCode(code))
		return B_HTTP_STATUS_CLASS_REDIRECTION;
	else if (BHttpRequest::IsClientErrorStatusCode(code))
		return B_HTTP_STATUS_CLASS_CLIENT_ERROR;
	else if (BHttpRequest::IsServerErrorStatusCode(code))
		return B_HTTP_STATUS_CLASS_SERVER_ERROR;

	return B_HTTP_STATUS_CLASS_INVALID;
}


const BUrlResult&
BHttpRequest::Result() const
{
	return fResult;
}


status_t
BHttpRequest::Stop()
{
	if (fSocket != NULL) {
		fSocket->Disconnect();
			// Unlock any pending connect, read or write operation.
	}
	return BNetworkRequest::Stop();
}


void
BHttpRequest::_ResetOptions()
{
	delete fOptPostFields;
	delete fOptHeaders;

	fOptFollowLocation = true;
	fOptMaxRedirs = 8;
	fOptReferer = "";
	fOptUserAgent = "Services Kit (Haiku)";
	fOptUsername = "";
	fOptPassword = "";
	fOptAuthMethods = B_HTTP_AUTHENTICATION_BASIC | B_HTTP_AUTHENTICATION_DIGEST
		| B_HTTP_AUTHENTICATION_IE_DIGEST;
	fOptHeaders = NULL;
	fOptPostFields = NULL;
	fOptSetCookies = true;
	fOptDiscardData = false;
	fOptDisableListener = false;
	fOptAutoReferer = true;
}


status_t
BHttpRequest::_ProtocolLoop()
{
	// Initialize the request redirection loop
	int8 maxRedirs = fOptMaxRedirs;
	bool newRequest;

	do {
		newRequest = false;

		// Result reset
		fHeaders.Clear();
		_ResultHeaders().Clear();

		BString host = fUrl.Host();
		int port = fSSL ? 443 : 80;

		if (fUrl.HasPort())
			port = fUrl.Port();

		if (fContext->UseProxy()) {
			host = fContext->GetProxyHost();
			port = fContext->GetProxyPort();
		}

		status_t result = fInputBuffer.InitCheck();
		if (result != B_OK)
			return result;

		if (!_ResolveHostName(host, port)) {
			_EmitDebug(B_URL_PROTOCOL_DEBUG_ERROR,
				"Unable to resolve hostname (%s), aborting.",
					fUrl.Host().String());
			return B_SERVER_NOT_FOUND;
		}

		status_t requestStatus = _MakeRequest();
		if (requestStatus != B_OK)
			return requestStatus;

		// Prepare the referer for the next request if needed
		if (fOptAutoReferer)
			fOptReferer = fUrl.UrlString();

		switch (StatusCodeClass(fResult.StatusCode())) {
			case B_HTTP_STATUS_CLASS_INFORMATIONAL:
				// Header 100:continue should have been
				// handled in the _MakeRequest read loop
				break;

			case B_HTTP_STATUS_CLASS_SUCCESS:
				break;

			case B_HTTP_STATUS_CLASS_REDIRECTION:
			{
				// Redirection has been explicitly disabled
				if (!fOptFollowLocation)
					break;

				int code = fResult.StatusCode();
				if (code == B_HTTP_STATUS_MOVED_PERMANENTLY
					|| code == B_HTTP_STATUS_FOUND
					|| code == B_HTTP_STATUS_SEE_OTHER
					|| code == B_HTTP_STATUS_TEMPORARY_REDIRECT) {
					BString locationUrl = fHeaders["Location"];

					fUrl = BUrl(fUrl, locationUrl);

					// 302 and 303 redirections also convert POST requests to GET
					// (and remove the posted form data)
					if ((code == B_HTTP_STATUS_FOUND
						|| code == B_HTTP_STATUS_SEE_OTHER)
						&& fRequestMethod == B_HTTP_POST) {
						SetMethod(B_HTTP_GET);
						delete fOptPostFields;
						fOptPostFields = NULL;
						delete fOptInputData;
						fOptInputData = NULL;
						fOptInputDataSize = 0;
					}

					if (--maxRedirs > 0) {
						newRequest = true;

						// Redirections may need a switch from http to https.
						if (fUrl.Protocol() == "https")
							fSSL = true;
						else if (fUrl.Protocol() == "http")
							fSSL = false;

						_EmitDebug(B_URL_PROTOCOL_DEBUG_TEXT,
							"Following: %s\n",
							fUrl.UrlString().String());
					}
				}
				break;
			}

			case B_HTTP_STATUS_CLASS_CLIENT_ERROR:
				if (fResult.StatusCode() == B_HTTP_STATUS_UNAUTHORIZED) {
					BHttpAuthentication* authentication
						= &fContext->GetAuthentication(fUrl);
					status_t status = B_OK;

					if (authentication->Method() == B_HTTP_AUTHENTICATION_NONE) {
						// There is no authentication context for this
						// url yet, so let's create one.
						BHttpAuthentication newAuth;
						newAuth.Initialize(fHeaders["WWW-Authenticate"]);
						fContext->AddAuthentication(fUrl, newAuth);

						// Get the copy of the authentication we just added.
						// That copy is owned by the BUrlContext and won't be
						// deleted (unlike the temporary object above)
						authentication = &fContext->GetAuthentication(fUrl);
					}

					newRequest = false;
					if (fOptUsername.Length() > 0 && status == B_OK) {
						// If we received an username and password, add them
						// to the request. This will either change the
						// credentials for an existing request, or set them
						// for a new one we created just above.
						//
						// If this request handles HTTP redirections, it will
						// also automatically retry connecting and send the
						// login information.
						authentication->SetUserName(fOptUsername);
						authentication->SetPassword(fOptPassword);
						newRequest = true;
					}
				}
				break;

			case B_HTTP_STATUS_CLASS_SERVER_ERROR:
				break;

			default:
			case B_HTTP_STATUS_CLASS_INVALID:
				break;
		}
	} while (newRequest);

	_EmitDebug(B_URL_PROTOCOL_DEBUG_TEXT,
		"%ld headers and %ld bytes of data remaining",
		fHeaders.CountHeaders(), fInputBuffer.Size());

	if (fResult.StatusCode() == 404)
		return B_RESOURCE_NOT_FOUND;

	return B_OK;
}


status_t
BHttpRequest::_MakeRequest()
{
	delete fSocket;

	if (fSSL) {
		if (fContext->UseProxy()) {
			BNetworkAddress proxy(fContext->GetProxyHost(), fContext->GetProxyPort());
			fSocket = new(std::nothrow) BPrivate::CheckedProxySecureSocket(proxy, this);
		} else
			fSocket = new(std::nothrow) BPrivate::CheckedSecureSocket(this);
	} else
		fSocket = new(std::nothrow) BSocket();

	if (fSocket == NULL)
		return B_NO_MEMORY;

	_EmitDebug(B_URL_PROTOCOL_DEBUG_TEXT, "Connection to %s on port %d.",
		fUrl.Authority().String(), fRemoteAddr.Port());
	status_t connectError = fSocket->Connect(fRemoteAddr);

	if (connectError != B_OK) {
		_EmitDebug(B_URL_PROTOCOL_DEBUG_ERROR, "Socket connection error %s",
			strerror(connectError));
		return connectError;
	}

	//! ProtocolHook:ConnectionOpened
	if (fListener != NULL)
		fListener->ConnectionOpened(this);

	_EmitDebug(B_URL_PROTOCOL_DEBUG_TEXT,
		"Connection opened, sending request.");

	BString requestHeaders;
	requestHeaders.Append(_SerializeRequest());
	requestHeaders.Append(_SerializeHeaders());
	requestHeaders.Append("\r\n");
	fSocket->Write(requestHeaders.String(), requestHeaders.Length());
	_EmitDebug(B_URL_PROTOCOL_DEBUG_TEXT, "Request sent.");

	_SendPostData();
	fRequestStatus = kRequestInitialState;



	// Receive loop
	bool receiveEnd = false;
	bool parseEnd = false;
	bool readByChunks = false;
	bool decompress = false;
	status_t readError = B_OK;
	ssize_t bytesRead = 0;
	off_t bytesReceived = 0;
	off_t bytesTotal = 0;
	size_t previousBufferSize = 0;
	off_t bytesUnpacked = 0;
	char* inputTempBuffer = new(std::nothrow) char[kHttpBufferSize];
	ArrayDeleter<char> inputTempBufferDeleter(inputTempBuffer);
	ssize_t inputTempSize = kHttpBufferSize;
	ssize_t chunkSize = -1;
	DynamicBuffer decompressorStorage;
	BDataIO* decompressingStream;
	ObjectDeleter<BDataIO> decompressingStreamDeleter;

	while (!fQuit && !(receiveEnd && parseEnd)) {
		if ((!receiveEnd) && (fInputBuffer.Size() == previousBufferSize)) {
			BStackOrHeapArray<char, 4096> chunk(kHttpBufferSize);
			bytesRead = fSocket->Read(chunk, kHttpBufferSize);

			if (bytesRead < 0) {
				readError = bytesRead;
				break;
			} else if (bytesRead == 0)
				receiveEnd = true;

			fInputBuffer.AppendData(chunk, bytesRead);
		} else
			bytesRead = 0;

		previousBufferSize = fInputBuffer.Size();

		if (fRequestStatus < kRequestStatusReceived) {
			_ParseStatus();

			//! ProtocolHook:ResponseStarted
			if (fRequestStatus >= kRequestStatusReceived && fListener != NULL)
				fListener->ResponseStarted(this);
		}

		if (fRequestStatus < kRequestHeadersReceived) {
			_ParseHeaders();

			if (fRequestStatus >= kRequestHeadersReceived) {
				_ResultHeaders() = fHeaders;

				// Parse received cookies
				if (fContext != NULL) {
					for (int32 i = 0;  i < fHeaders.CountHeaders(); i++) {
						if (fHeaders.HeaderAt(i).NameIs("Set-Cookie")) {
							fContext->GetCookieJar().AddCookie(
								fHeaders.HeaderAt(i).Value(), fUrl);
						}
					}
				}

				//! ProtocolHook:HeadersReceived
				if (fListener != NULL)
					fListener->HeadersReceived(this, fResult);


				if (BString(fHeaders["Transfer-Encoding"]) == "chunked")
					readByChunks = true;

				BString contentEncoding(fHeaders["Content-Encoding"]);
				// We don't advertise "deflate" support (see above), but we
				// still try to decompress it, if a server ever sends a deflate
				// stream despite it not being in our Accept-Encoding list.
				if (contentEncoding == "gzip"
						|| contentEncoding == "deflate") {
					decompress = true;
					readError = BZlibCompressionAlgorithm()
						.CreateDecompressingOutputStream(&decompressorStorage,
							NULL, decompressingStream);
					if (readError != B_OK)
						break;

					decompressingStreamDeleter.SetTo(decompressingStream);
				}

				int32 index = fHeaders.HasHeader("Content-Length");
				if (index != B_ERROR)
					bytesTotal = atoll(fHeaders.HeaderAt(index).Value());
				else
					bytesTotal = -1;
			}
		}

		if (fRequestStatus >= kRequestHeadersReceived) {
			// If Transfer-Encoding is chunked, we should read a complete
			// chunk in buffer before handling it
			if (readByChunks) {
				if (chunkSize >= 0) {
					if ((ssize_t)fInputBuffer.Size() >= chunkSize + 2) {
							// 2 more bytes to handle the closing CR+LF
						bytesRead = chunkSize;
						if (inputTempSize < chunkSize + 2) {
							inputTempSize = chunkSize + 2;
							inputTempBuffer
								= new(std::nothrow) char[inputTempSize];
							inputTempBufferDeleter.SetTo(inputTempBuffer);
						}

						if (inputTempBuffer == NULL) {
							readError = B_NO_MEMORY;
							break;
						}

						fInputBuffer.RemoveData(inputTempBuffer,
							chunkSize + 2);
						chunkSize = -1;
					} else {
						// Not enough data, try again later
						bytesRead = -1;
					}
				} else {
					BString chunkHeader;
					if (_GetLine(chunkHeader) == B_ERROR) {
						chunkSize = -1;
						bytesRead = -1;
					} else {
						// Format of a chunk header:
						// <chunk size in hex>[; optional data]
						int32 semiColonIndex = chunkHeader.FindFirst(';', 0);

						// Cut-off optional data if present
						if (semiColonIndex != -1) {
							chunkHeader.Remove(semiColonIndex,
								chunkHeader.Length() - semiColonIndex);
						}

						chunkSize = strtol(chunkHeader.String(), NULL, 16);
						if (chunkSize == 0)
							fRequestStatus = kRequestContentReceived;

						bytesRead = -1;
					}
				}

				// A chunk of 0 bytes indicates the end of the chunked transfer
				if (bytesRead == 0)
					receiveEnd = true;
			} else {
				bytesRead = fInputBuffer.Size();

				if (bytesRead > 0) {
					if (inputTempSize < bytesRead) {
						inputTempSize = bytesRead;
						inputTempBuffer = new(std::nothrow) char[bytesRead];
						inputTempBufferDeleter.SetTo(inputTempBuffer);
					}

					if (inputTempBuffer == NULL) {
						readError = B_NO_MEMORY;
						break;
					}
					fInputBuffer.RemoveData(inputTempBuffer, bytesRead);
				}
			}

			if (bytesRead >= 0) {
				bytesReceived += bytesRead;

				if (fListener != NULL) {
					if (decompress) {
						readError = decompressingStream->WriteExactly(
							inputTempBuffer, bytesRead);
						if (readError != B_OK)
							break;

						ssize_t size = decompressorStorage.Size();
						BStackOrHeapArray<char, 4096> buffer(size);
						size = decompressorStorage.Read(buffer, size);
						if (size > 0) {
							fListener->DataReceived(this, buffer, bytesUnpacked,
								size);
							bytesUnpacked += size;
						}
					} else if (bytesRead > 0) {
						fListener->DataReceived(this, inputTempBuffer,
							bytesReceived - bytesRead, bytesRead);
					}
					fListener->DownloadProgress(this, bytesReceived,
						std::max((off_t)0, bytesTotal));
				}

				if (bytesTotal >= 0 && bytesReceived >= bytesTotal)
					receiveEnd = true;

				if (decompress && receiveEnd) {
					readError = decompressingStream->Flush();

					if (readError == B_BUFFER_OVERFLOW)
						readError = B_OK;

					if (readError != B_OK)
						break;

					ssize_t size = decompressorStorage.Size();
					BStackOrHeapArray<char, 4096> buffer(size);
					size = decompressorStorage.Read(buffer, size);
					if (fListener != NULL && size > 0) {
						fListener->DataReceived(this, buffer,
							bytesUnpacked, size);
						bytesUnpacked += size;
					}
				}
			}
		}

		parseEnd = (fInputBuffer.Size() == 0);
	}

	fSocket->Disconnect();

	if (readError != B_OK)
		return readError;

	return fQuit ? B_INTERRUPTED : B_OK;
}


void
BHttpRequest::_ParseStatus()
{
	// Status line should be formatted like: HTTP/M.m SSS ...
	// With:   M = Major version of the protocol
	//         m = Minor version of the protocol
	//       SSS = three-digit status code of the response
	//       ... = additional text info
	BString statusLine;
	if (_GetLine(statusLine) == B_ERROR)
		return;

	if (statusLine.CountChars() < 12)
		return;

	fRequestStatus = kRequestStatusReceived;

	BString statusCodeStr;
	BString statusText;
	statusLine.CopyInto(statusCodeStr, 9, 3);
	_SetResultStatusCode(atoi(statusCodeStr.String()));

	statusLine.CopyInto(_ResultStatusText(), 13, statusLine.Length() - 13);

	_EmitDebug(B_URL_PROTOCOL_DEBUG_TEXT, "Status line received: Code %d (%s)",
		atoi(statusCodeStr.String()), _ResultStatusText().String());
}


void
BHttpRequest::_ParseHeaders()
{
	BString currentHeader;
	while (_GetLine(currentHeader) != B_ERROR) {
		// An empty line means the end of the header section
		if (currentHeader.Length() == 0) {
			fRequestStatus = kRequestHeadersReceived;
			return;
		}

		_EmitDebug(B_URL_PROTOCOL_DEBUG_HEADER_IN, "%s",
			currentHeader.String());
		fHeaders.AddHeader(currentHeader.String());
	}
}


BString
BHttpRequest::_SerializeRequest()
{
	BString request(fRequestMethod);
	request << ' ';

	if (fContext->UseProxy()) {
		// When there is a proxy, the request must include the host and port so
		// the proxy knows where to send the request.
		request << Url().Protocol() << "://" << Url().Host();
		if (Url().HasPort())
			request << ':' << Url().Port();
	}

	if (Url().HasPath() && Url().Path().Length() > 0)
		request << Url().Path();
	else
		request << '/';

	if (Url().HasRequest())
		request << '?' << Url().Request();

	switch (fHttpVersion) {
		case B_HTTP_11:
			request << " HTTP/1.1\r\n";
			break;

		default:
		case B_HTTP_10:
			request << " HTTP/1.0\r\n";
			break;
	}

	_EmitDebug(B_URL_PROTOCOL_DEBUG_HEADER_OUT, "%s", request.String());

	return request;
}


BString
BHttpRequest::_SerializeHeaders()
{
	BHttpHeaders outputHeaders;

	// HTTP 1.1 additional headers
	if (fHttpVersion == B_HTTP_11) {
		BString host = Url().Host();
		if (Url().HasPort() && !_IsDefaultPort())
			host << ':' << Url().Port();

		outputHeaders.AddHeader("Host", host);

		outputHeaders.AddHeader("Accept", "*/*");
		outputHeaders.AddHeader("Accept-Encoding", "gzip");
			// Allows the server to compress data using the "gzip" format.
			// "deflate" is not supported, because there are two interpretations
			// of what it means (the RFC and Microsoft products), and we don't
			// want to handle this. Very few websites support only deflate,
			// and most of them will send gzip, or at worst, uncompressed data.

		outputHeaders.AddHeader("Connection", "close");
			// Let the remote server close the connection after response since
			// we don't handle multiple request on a single connection
	}

	// Classic HTTP headers
	if (fOptUserAgent.CountChars() > 0)
		outputHeaders.AddHeader("User-Agent", fOptUserAgent.String());

	if (fOptReferer.CountChars() > 0)
		outputHeaders.AddHeader("Referer", fOptReferer.String());

	// Authentication
	if (fContext != NULL) {
		BHttpAuthentication& authentication = fContext->GetAuthentication(fUrl);
		if (authentication.Method() != B_HTTP_AUTHENTICATION_NONE) {
			if (fOptUsername.Length() > 0) {
				authentication.SetUserName(fOptUsername);
				authentication.SetPassword(fOptPassword);
			}

			BString request(fRequestMethod);
			outputHeaders.AddHeader("Authorization",
				authentication.Authorization(fUrl, request));
		}
	}

	// Required headers for POST data
	if (fOptPostFields != NULL && fRequestMethod == B_HTTP_POST) {
		BString contentType;

		switch (fOptPostFields->GetFormType()) {
			case B_HTTP_FORM_MULTIPART:
				contentType << "multipart/form-data; boundary="
					<< fOptPostFields->GetMultipartBoundary() << "";
				break;

			case B_HTTP_FORM_URL_ENCODED:
				contentType << "application/x-www-form-urlencoded";
				break;
		}

		outputHeaders.AddHeader("Content-Type", contentType);
		outputHeaders.AddHeader("Content-Length",
			fOptPostFields->ContentLength());
	} else if (fOptInputData != NULL
			&& (fRequestMethod == B_HTTP_POST
			|| fRequestMethod == B_HTTP_PUT)) {
		if (fOptInputDataSize >= 0)
			outputHeaders.AddHeader("Content-Length", fOptInputDataSize);
		else
			outputHeaders.AddHeader("Transfer-Encoding", "chunked");
	}

	// Optional headers specified by the user
	if (fOptHeaders != NULL) {
		for (int32 headerIndex = 0; headerIndex < fOptHeaders->CountHeaders();
				headerIndex++) {
			BHttpHeader& optHeader = (*fOptHeaders)[headerIndex];
			int32 replaceIndex = outputHeaders.HasHeader(optHeader.Name());

			// Add or replace the current option header to the
			// output header list
			if (replaceIndex == -1)
				outputHeaders.AddHeader(optHeader.Name(), optHeader.Value());
			else
				outputHeaders[replaceIndex].SetValue(optHeader.Value());
		}
	}

	// Context cookies
	if (fOptSetCookies && fContext != NULL) {
		BString cookieString;

		BNetworkCookieJar::UrlIterator iterator
			= fContext->GetCookieJar().GetUrlIterator(fUrl);
		const BNetworkCookie* cookie = iterator.Next();
		if (cookie != NULL) {
			while (true) {
				cookieString << cookie->RawCookie(false);
				cookie = iterator.Next();
				if (cookie == NULL)
					break;
				cookieString << "; ";
			}

			outputHeaders.AddHeader("Cookie", cookieString);
		}
	}

	// Write output headers to output stream
	BString headerData;

	for (int32 headerIndex = 0; headerIndex < outputHeaders.CountHeaders();
			headerIndex++) {
		const char* header = outputHeaders.HeaderAt(headerIndex).Header();

		headerData << header;
		headerData << "\r\n";

		_EmitDebug(B_URL_PROTOCOL_DEBUG_HEADER_OUT, "%s", header);
	}

	return headerData;
}


void
BHttpRequest::_SendPostData()
{
	if (fRequestMethod == B_HTTP_POST && fOptPostFields != NULL) {
		if (fOptPostFields->GetFormType() != B_HTTP_FORM_MULTIPART) {
			BString outputBuffer = fOptPostFields->RawData();
			_EmitDebug(B_URL_PROTOCOL_DEBUG_TRANSFER_OUT,
				"%s", outputBuffer.String());
			fSocket->Write(outputBuffer.String(), outputBuffer.Length());
		} else {
			for (BHttpForm::Iterator it = fOptPostFields->GetIterator();
				const BHttpFormData* currentField = it.Next();
				) {
				_EmitDebug(B_URL_PROTOCOL_DEBUG_TRANSFER_OUT,
					it.MultipartHeader().String());
				fSocket->Write(it.MultipartHeader().String(),
					it.MultipartHeader().Length());

				switch (currentField->Type()) {
					default:
					case B_HTTPFORM_UNKNOWN:
						ASSERT(0);
						break;

					case B_HTTPFORM_STRING:
						fSocket->Write(currentField->String().String(),
							currentField->String().Length());
						break;

					case B_HTTPFORM_FILE:
						{
							BFile upFile(currentField->File().Path(),
								B_READ_ONLY);
							char readBuffer[kHttpBufferSize];
							ssize_t readSize;
							off_t totalSize;

							if (upFile.GetSize(&totalSize) != B_OK)
								ASSERT(0);

							readSize = upFile.Read(readBuffer,
								sizeof(readBuffer));
							while (readSize > 0) {
								fSocket->Write(readBuffer, readSize);
								readSize = upFile.Read(readBuffer,
									sizeof(readBuffer));
								fListener->UploadProgress(this, readSize,
									std::max((off_t)0, totalSize));
							}

							break;
						}
					case B_HTTPFORM_BUFFER:
						fSocket->Write(currentField->Buffer(),
							currentField->BufferSize());
						break;
				}

				fSocket->Write("\r\n", 2);
			}

			BString footer = fOptPostFields->GetMultipartFooter();
			fSocket->Write(footer.String(), footer.Length());
		}
	} else if ((fRequestMethod == B_HTTP_POST || fRequestMethod == B_HTTP_PUT)
		&& fOptInputData != NULL) {

		// If the input data is seekable, we rewind it for each new request.
		BPositionIO* seekableData
			= dynamic_cast<BPositionIO*>(fOptInputData);
		if (seekableData)
			seekableData->Seek(0, SEEK_SET);

		for (;;) {
			char outputTempBuffer[kHttpBufferSize];
			ssize_t read = fOptInputData->Read(outputTempBuffer,
				sizeof(outputTempBuffer));

			if (read <= 0)
				break;

			if (fOptInputDataSize < 0) {
				// Input data size unknown, so we have to use chunked transfer
				char hexSize[18];
				// The string does not need to be NULL terminated.
				size_t hexLength = snprintf(hexSize, sizeof(hexSize), "%lx\r\n",
					read);

				fSocket->Write(hexSize, hexLength);
				fSocket->Write(outputTempBuffer, read);
				fSocket->Write("\r\n", 2);
			} else {
				fSocket->Write(outputTempBuffer, read);
			}
		}

		if (fOptInputDataSize < 0) {
			// Chunked transfer terminating sequence
			fSocket->Write("0\r\n\r\n", 5);
		}
	}

}


BHttpHeaders&
BHttpRequest::_ResultHeaders()
{
	return fResult.fHeaders;
}


void
BHttpRequest::_SetResultStatusCode(int32 statusCode)
{
	fResult.fStatusCode = statusCode;
}


BString&
BHttpRequest::_ResultStatusText()
{
	return fResult.fStatusString;
}


bool
BHttpRequest::_CertificateVerificationFailed(BCertificate& certificate,
	const char* message)
{
	if (fContext->HasCertificateException(certificate))
		return true;

	if (fListener != NULL
		&& fListener->CertificateVerificationFailed(this, certificate, message)) {
		// User asked us to continue anyway, let's add a temporary exception for this certificate
		fContext->AddCertificateException(certificate);
		return true;
	}

	return false;
}


bool
BHttpRequest::_IsDefaultPort()
{
	if (fSSL && Url().Port() == 443)
		return true;
	if (!fSSL && Url().Port() == 80)
		return true;
	return false;
}


