/*
 * Copyright 2010-2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_PROTOCOL_HTTP_H_
#define _B_URL_PROTOCOL_HTTP_H_


#include <deque>

#include <Certificate.h>
#include <HttpForm.h>
#include <HttpHeaders.h>
#include <HttpResult.h>
#include <NetworkAddress.h>
#include <NetworkRequest.h>
#include <UrlProtocolRoster.h>


namespace BPrivate {
	class CheckedSecureSocket;
	class CheckedProxySecureSocket;
};


class BHttpRequest : public BNetworkRequest {
public:
	virtual						~BHttpRequest();

			void				SetMethod(const char* const method);
			void				SetFollowLocation(bool follow);
			void				SetMaxRedirections(int8 maxRedirections);
			void				SetReferrer(const BString& referrer);
			void				SetUserAgent(const BString& agent);
			void				SetDiscardData(bool discard);
			void				SetDisableListener(bool disable);
			void				SetAutoReferrer(bool enable);
			void				SetUserName(const BString& name);
			void				SetPassword(const BString& password);
			void				SetRangeStart(off_t position);
			void				SetRangeEnd(off_t position);

			void				SetPostFields(const BHttpForm& fields);
			void				SetHeaders(const BHttpHeaders& headers);

			void				AdoptPostFields(BHttpForm* const fields);
			void				AdoptInputData(BDataIO* const data,
									const ssize_t size = -1);
			void				AdoptHeaders(BHttpHeaders* const headers);

			status_t			Stop();
			const BUrlResult&	Result() const;

	static	bool				IsInformationalStatusCode(int16 code);
	static	bool				IsSuccessStatusCode(int16 code);
	static	bool				IsRedirectionStatusCode(int16 code);
	static	bool				IsClientErrorStatusCode(int16 code);
	static	bool				IsServerErrorStatusCode(int16 code);
	static	int16				StatusCodeClass(int16 code);

private:
			friend class BUrlProtocolRoster;

								BHttpRequest(const BUrl& url,
									bool ssl = false,
									const char* protocolName = "HTTP",
									BUrlProtocolListener* listener = NULL,
									BUrlContext* context = NULL);
								BHttpRequest(const BHttpRequest& other);

			void				_ResetOptions();
			status_t			_ProtocolLoop();
			status_t			_MakeRequest();

			BString				_SerializeRequest();
			BString				_SerializeHeaders();
			void				_SendPostData();

			void				_ParseStatus();
			void				_ParseHeaders();

	// URL result parameters access
			BPositionIO*		_ResultRawData();
			BHttpHeaders&		_ResultHeaders();
			void				_SetResultStatusCode(int32 statusCode);
			BString&			_ResultStatusText();

	// SSL failure management
	friend	class				BPrivate::CheckedSecureSocket;
	friend	class				BPrivate::CheckedProxySecureSocket;
			bool				_CertificateVerificationFailed(
									BCertificate& certificate,
									const char* message);

	// Utility methods
			bool				_IsDefaultPort();

private:
			bool				fSSL;

			BString				fRequestMethod;
			int8				fHttpVersion;

			BHttpHeaders		fHeaders;

	// Request status

			BHttpResult			fResult;

			// Request state/events
			enum {
				kRequestInitialState,
				kRequestStatusReceived,
				kRequestHeadersReceived,
				kRequestContentReceived,
				kRequestTrailingHeadersReceived
			}					fRequestStatus;


	// Protocol options
			uint8				fOptMaxRedirs;
			BString				fOptReferer;
			BString				fOptUserAgent;
			BString				fOptUsername;
			BString				fOptPassword;
			uint32				fOptAuthMethods;
			BHttpHeaders*		fOptHeaders;
			BHttpForm*			fOptPostFields;
			BDataIO*			fOptInputData;
			ssize_t				fOptInputDataSize;
			off_t				fOptRangeStart;
			off_t				fOptRangeEnd;
			bool				fOptSetCookies : 1;
			bool				fOptFollowLocation : 1;
			bool				fOptDiscardData : 1;
			bool				fOptDisableListener : 1;
			bool				fOptAutoReferer : 1;
};

// Request method
const char* const B_HTTP_GET = "GET";
const char* const B_HTTP_POST = "POST";
const char* const B_HTTP_PUT = "PUT";
const char* const B_HTTP_HEAD = "HEAD";
const char* const B_HTTP_DELETE = "DELETE";
const char* const B_HTTP_OPTIONS = "OPTIONS";
const char* const B_HTTP_TRACE = "TRACE";
const char* const B_HTTP_CONNECT = "CONNECT";


// HTTP Version
enum {
	B_HTTP_10 = 1,
	B_HTTP_11
};


// HTTP status classes
enum http_status_code_class {
	B_HTTP_STATUS_CLASS_INVALID			= 000,
	B_HTTP_STATUS_CLASS_INFORMATIONAL 	= 100,
	B_HTTP_STATUS_CLASS_SUCCESS			= 200,
	B_HTTP_STATUS_CLASS_REDIRECTION		= 300,
	B_HTTP_STATUS_CLASS_CLIENT_ERROR	= 400,
	B_HTTP_STATUS_CLASS_SERVER_ERROR	= 500
};


// Known HTTP status codes
enum http_status_code {
	// Informational status codes
	B_HTTP_STATUS__INFORMATIONAL_BASE	= 100,
	B_HTTP_STATUS_CONTINUE = B_HTTP_STATUS__INFORMATIONAL_BASE,
	B_HTTP_STATUS_SWITCHING_PROTOCOLS,
	B_HTTP_STATUS__INFORMATIONAL_END,

	// Success status codes
	B_HTTP_STATUS__SUCCESS_BASE			= 200,
	B_HTTP_STATUS_OK = B_HTTP_STATUS__SUCCESS_BASE,
	B_HTTP_STATUS_CREATED,
	B_HTTP_STATUS_ACCEPTED,
	B_HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION,
	B_HTTP_STATUS_NO_CONTENT,
	B_HTTP_STATUS_RESET_CONTENT,
	B_HTTP_STATUS_PARTIAL_CONTENT,
	B_HTTP_STATUS__SUCCESS_END,

	// Redirection status codes
	B_HTTP_STATUS__REDIRECTION_BASE		= 300,
	B_HTTP_STATUS_MULTIPLE_CHOICE = B_HTTP_STATUS__REDIRECTION_BASE,
	B_HTTP_STATUS_MOVED_PERMANENTLY,
	B_HTTP_STATUS_FOUND,
	B_HTTP_STATUS_SEE_OTHER,
	B_HTTP_STATUS_NOT_MODIFIED,
	B_HTTP_STATUS_USE_PROXY,
	B_HTTP_STATUS_TEMPORARY_REDIRECT,
	B_HTTP_STATUS__REDIRECTION_END,

	// Client error status codes
	B_HTTP_STATUS__CLIENT_ERROR_BASE	= 400,
	B_HTTP_STATUS_BAD_REQUEST = B_HTTP_STATUS__CLIENT_ERROR_BASE,
	B_HTTP_STATUS_UNAUTHORIZED,
	B_HTTP_STATUS_PAYMENT_REQUIRED,
	B_HTTP_STATUS_FORBIDDEN,
	B_HTTP_STATUS_NOT_FOUND,
	B_HTTP_STATUS_METHOD_NOT_ALLOWED,
	B_HTTP_STATUS_NOT_ACCEPTABLE,
	B_HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED,
	B_HTTP_STATUS_REQUEST_TIMEOUT,
	B_HTTP_STATUS_CONFLICT,
	B_HTTP_STATUS_GONE,
	B_HTTP_STATUS_LENGTH_REQUIRED,
	B_HTTP_STATUS_PRECONDITION_FAILED,
	B_HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE,
	B_HTTP_STATUS_REQUEST_URI_TOO_LARGE,
	B_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,
	B_HTTP_STATUS_REQUESTED_RANGE_NOT_SATISFIABLE,
	B_HTTP_STATUS_EXPECTATION_FAILED,
	B_HTTP_STATUS__CLIENT_ERROR_END,

	// Server error status codes
	B_HTTP_STATUS__SERVER_ERROR_BASE 	= 500,
	B_HTTP_STATUS_INTERNAL_SERVER_ERROR = B_HTTP_STATUS__SERVER_ERROR_BASE,
	B_HTTP_STATUS_NOT_IMPLEMENTED,
	B_HTTP_STATUS_BAD_GATEWAY,
	B_HTTP_STATUS_SERVICE_UNAVAILABLE,
	B_HTTP_STATUS_GATEWAY_TIMEOUT,
	B_HTTP_STATUS__SERVER_ERROR_END
};


// HTTP default User-Agent
#define B_HTTP_PROTOCOL_USER_AGENT_FORMAT "ServicesKit (%s)"

#endif // _B_URL_PROTOCOL_HTTP_H_
