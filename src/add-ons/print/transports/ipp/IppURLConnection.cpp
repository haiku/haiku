// Sun, 18 Jun 2000
// Y.Takagi

#if defined(__HAIKU__) || defined(HAIKU_TARGET_PLATFORM_BONE)
#	include <sys/socket.h>
#else
#	include <net/socket.h>
#endif
#include <iostream>
#include <strings.h>
#include <stdio.h>
char *itoa(int i, char *buf, int unit)
{
	sprintf(buf, "%d", i);
	return buf;
}
#define stricmp		strcasecmp
#define strnicmp	strncasecmp

#include <list>
#include "IppURLConnection.h"
#include "IppContent.h"

IppURLConnection::IppURLConnection(const BUrl &Url)
	: HttpURLConnection(Url)
{
	__ippRequest  = NULL;
	__ippResponse = new IppContent;
	setRequestMethod("POST");
	setRequestProperty("Content-Type", "application/ipp");
	setRequestProperty("Cache-Control", "no-cache");
	setRequestProperty("Pragma", "no-cache");
}

IppURLConnection::~IppURLConnection()
{
	if (__ippRequest) {
		delete __ippRequest;
	}

	if (__ippResponse) {
		delete __ippResponse;
	}
}

void IppURLConnection::setIppRequest(IppContent *obj)
{
	if (__ippRequest) {
		delete __ippRequest;
	}
	__ippRequest = obj;
}


const IppContent *IppURLConnection::getIppResponse() const
{
	return __ippResponse;
}

void IppURLConnection::setRequest()
{
	if (connected) {
		char buf[64];
		itoa(__ippRequest->length(), buf, 10);
		setRequestProperty("Content-Length", buf);
		HttpURLConnection::setRequest();
	}
}

void IppURLConnection::setContent()
{
	if (connected && __ippRequest) {
		ostream &os = getOutputStream();
		os << *__ippRequest;
	}
}

inline bool is_contenttype_ipp(const char *s)
{
	return strnicmp(s, "application/ipp", 15) ? false : true;
}

void IppURLConnection::getContent()
{
	if (connected) {
		if (getResponseCode() == HTTP_OK && is_contenttype_ipp(getContentType())) {
			istream &is = getInputStream();
			is >> *__ippResponse;
		} else {
			HttpURLConnection::getContent();
		}
	}
}

ostream &IppURLConnection::printIppRequest(ostream &os)
{
	return __ippRequest->print(os);
}

ostream &IppURLConnection::printIppResponse(ostream &os)
{
	if (getResponseCode() == HTTP_OK && is_contenttype_ipp(getContentType())) {
		return __ippResponse->print(os);
	}
	return os;
}
