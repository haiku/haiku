/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_PROTOCOL_HTTP_H_
#define _B_URL_PROTOCOL_HTTP_H_


#include <deque>


#include <HttpAuthentication.h>
#include <HttpForm.h>
#include <HttpHeaders.h>
#include <NetBuffer.h>
#include <NetworkAddress.h>
#include <UrlRequest.h>


class BAbstractSocket;


class BHttpRequest : public BUrlRequest {
public:
								BHttpRequest(const BUrl& url,
									BUrlResult& result, bool ssl = false,
									const char *protocolName = "HTTP",
									BUrlProtocolListener* listener = NULL,
									BUrlContext* context = NULL);
	virtual						~BHttpRequest();

            void                SetMethod(int8 method);
            void                SetFollowLocation(bool follow);
            void                SetMaxRedirections(int8 maxRedirections);
            void                SetReferrer(const BString& referrer);
            void                SetUserAgent(const BString& agent);
            void                SetHeaders(BHttpHeaders* headers);
            void                SetDiscardData(bool discard);
            void                SetDisableListener(bool disable);
            void                SetAutoReferrer(bool enable);
            void                SetPostFields(BHttpForm* fields);
            void                SetInputData(BDataIO* data, ssize_t size = -1);
            void                SetUserName(const BString& name);
            void                SetPassword(const BString& password);

	static	bool				IsInformationalStatusCode(int16 code);
	static	bool				IsSuccessStatusCode(int16 code);
	static	bool				IsRedirectionStatusCode(int16 code);
	static	bool				IsClientErrorStatusCode(int16 code);
	static	bool				IsServerErrorStatusCode(int16 code);
	static	int16				StatusCodeClass(int16 code);

	virtual	const char*			StatusString(status_t threadStatus) const;

private:
			void				_ResetOptions();
			status_t			_ProtocolLoop();
			bool 				_ResolveHostName();
			status_t			_MakeRequest();

			void				_CreateRequest();
			void				_AddHeaders();

			status_t			_GetLine(BString& destString);

			void				_ParseStatus();
			void				_ParseHeaders();

			void				_AddOutputBufferLine(const char* line);


private:
			BAbstractSocket*	fSocket;
			BNetworkAddress		fRemoteAddr;
			bool				fSSL;

			int8				fRequestMethod;
			int8				fHttpVersion;

			BString				fOutputBuffer;
			BNetBuffer			fInputBuffer;

			BHttpHeaders		fHeaders;
			BHttpAuthentication	fAuthentication;

	// Request status

			BHttpHeaders		fOutputHeaders;

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
			bool				fOptSetCookies : 1;
			bool				fOptFollowLocation : 1;
			bool				fOptDiscardData : 1;
			bool				fOptDisableListener : 1;
			bool				fOptAutoReferer : 1;
};

// ProtocolLoop return status
enum {
	B_PROT_HTTP_NOT_FOUND = B_PROT_THREAD_STATUS__END,
	B_PROT_HTTP_THREAD_STATUS__END
};


// Request method
enum {
	B_HTTP_GET = 1,
	B_HTTP_POST,
	B_HTTP_PUT,
	B_HTTP_HEAD,
	B_HTTP_DELETE,
	B_HTTP_OPTIONS
};


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
