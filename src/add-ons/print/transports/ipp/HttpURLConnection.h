// Sun, 18 Jun 2000
// Y.Takagi

#ifndef __HttpURLConnection_H
#define __HttpURLConnection_H

#include <iostream>

#include <list>
#include <string>

#include <Url.h>

using namespace std;

class Socket;

enum HTTP_RESPONSECODE {
	HTTP_UNKNOWN		= -1,	//

	HTTP_CONTINUE		= 100,	// Everything OK, keep going...
	HTTP_SWITCH_PROC	= 101,	// Switching Protocols

	HTTP_OK				= 200,	// OPTIONS/GET/HEAD/POST/TRACE command was successful
	HTTP_CREATED,				// PUT command was successful
	HTTP_ACCEPTED,				// DELETE command was successful
	HTTP_NOT_AUTHORITATIVE,		// Information isn't authoritative
	HTTP_NO_CONTENT,			// Successful command, no new data
	HTTP_RESET,					// Content was reset/recreated
	HTTP_PARTIAL,				// Only a partial file was recieved/sent

	HTTP_MULTI_CHOICE	= 300,	// Multiple files match request
	HTTP_MOVED_PERM,			// Document has moved permanently
	HTTP_MOVED_TEMP,			// Document has moved temporarily
	HTTP_SEE_OTHER,				// See this other link...
	HTTP_NOT_MODIFIED,			// File not modified
	HTTP_USE_PROXY,				// Must use a proxy to access this URI

	HTTP_BAD_REQUEST	= 400,	// Bad request
	HTTP_UNAUTHORIZED,			// Unauthorized to access host
	HTTP_PAYMENT_REQUIRED,		// Payment required
	HTTP_FORBIDDEN,				// Forbidden to access this URI
	HTTP_NOT_FOUND,				// URI was not found
	HTTP_BAD_METHOD,			// Method is not allowed
	HTTP_NOT_ACCEPTABLE,		// Not Acceptable
	HTTP_PROXY_AUTH,			// Proxy Authentication is Required
	HTTP_REQUEST_TIMEOUT,		// Request timed out
	HTTP_CONFLICT,				// Request is self-conflicting
	HTTP_GONE,					// Server has gone away
	HTTP_LENGTH_REQUIRED,		// A content length or encoding is required
	HTTP_PRECON_FAILED,			// Precondition failed
	HTTP_ENTITY_TOO_LARGE,		// URI too long
	HTTP_REQ_TOO_LONG,			// Request entity too large
	HTTP_UNSUPPORTED_TYPE,		// The requested media type is unsupported

	HTTP_SERVER_ERROR	= 500,	// Internal server error
	HTTP_INTERNAL_ERROR,		// Feature not implemented
	HTTP_BAD_GATEWAY,			// Bad gateway
	HTTP_UNAVAILABLE,			// Service is unavailable
	HTTP_GATEWAY_TIMEOUT,		// Gateway connection timed out
	HTTP_VERSION				// HTTP version not supported
};

struct Field {
	string key;
	string value;
	Field() {}
	Field(char *field);
	Field(const char *k, const char *v);
	Field(const Field &);
	Field &operator = (const Field &);
	bool operator == (const Field &);
};

typedef list<Field>	Fields;

class HttpURLConnection {
public:
	HttpURLConnection(const BUrl &url);
	virtual ~HttpURLConnection();

	virtual void connect();
	void disconnect();

	void setRequestMethod(const char *method);
	const char *getRequestMethod() const;
	void setRequestProperty(const char *key, const char *value);

	const char *getContentType();
	const char *getContentEncoding();
	int getContentLength();
	long getDate();
	const char *getHeaderField(int n);
	const char *getHeaderField(const char *);
	const BUrl &getURL() const;
	HTTP_RESPONSECODE getResponseCode();
	const char *getResponseMessage();

	bool getDoInput() const;
	bool getDoOutput() const;
	void setDoInput(bool doInput);
	void setDoOutput(bool doOutput);

	istream &getInputStream();
	ostream &getOutputStream();

	const char *getLastError() const;
	void setLastError(const char *);

protected:
	bool connected;
	bool doInput;
	bool doOutput;
	BUrl  url;

	virtual void action();
	virtual void setRequest();
	virtual void setContent();
	virtual void getResponse();
	virtual void getContent();

private:
	Fields *__request;
	Fields *__response;
	Socket *__sock;
	string __method;
	string __response_message;
	HTTP_RESPONSECODE __response_code;
	string __error_msg;
};

inline const char *HttpURLConnection::getLastError() const
{
	return __error_msg.c_str();
}

inline void HttpURLConnection::setLastError(const char *e)
{
	__error_msg = e;
}

#endif	// __HttpURLConnection_H
