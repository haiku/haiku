// Sun, 18 Jun 2000
// Y.Takagi

#include <list>
#include <algorithm>
#include <cstring>
#include <strings.h>

#define stricmp	strcasecmp

#include "HttpURLConnection.h"
#include "Socket.h"

using namespace std;

#define DEFAULT_PORT	80;

Field::Field(char *field)
{
	char *p = strtok(field, ": \t\r\n");
	key = p ? p : "";
	p = strtok(NULL,   " \t\r\n");
	value = p ? p : "";
}

Field::Field(const char *k, const char *v)
{
	key   = k ? k : "";
	value = v ? v : "";
}

Field::Field(const Field &o)
{
	key   = o.key;
	value = o.value;
}

Field &Field::operator = (const Field &o)
{
	key   = o.key;
	value = o.value;
	return *this;
}

bool Field::operator == (const Field &o)
{
	return (key == o.key) && (value == o.value);
}

HttpURLConnection::HttpURLConnection(const BUrl &Url)
	: connected(false), doInput(true), doOutput(false), url(Url)
{
	__sock     = NULL;
	__method   = "GET";
	__request  = NULL;
	__response = NULL;
	__response_code = HTTP_UNKNOWN;
}

HttpURLConnection::~HttpURLConnection()
{
	disconnect();

	if (__sock) {
		delete __sock;
	}

	if (__request) {
		delete __request;
	}

	if (__response) {
		delete __response;
	}
}

void HttpURLConnection::disconnect()
{
	if (connected) {
		connected = false;
		__sock->close();
	}
}

const char *HttpURLConnection::getRequestMethod() const
{
	return __method.c_str();
}

void HttpURLConnection::setRequestMethod(const char *method)
{
	__method = method;
}

void HttpURLConnection::setRequestProperty(const char *key, const char *value)
{
	if (__request == NULL) {
		__request = new Fields;
	}
	__request->push_back(Field(key, value));
}

istream &HttpURLConnection::getInputStream()
{
	if (!connected) {
		connect();
		setRequest();
	}
	return __sock->getInputStream();
}

ostream &HttpURLConnection::getOutputStream()
{
	if (!connected) {
		connect();
	}
	return __sock->getOutputStream();
}

void HttpURLConnection::setDoInput(bool doInput)
{
	this->doInput = doInput;
}

void HttpURLConnection::setDoOutput(bool doOutput)
{
	this->doOutput = doOutput;
}

void HttpURLConnection::connect()
{
	if (!connected) {
		int port = url.Port();
		if (port < 0) {
			const char *protocol = url.Protocol();
			if (!stricmp(protocol, "http")) {
				port = DEFAULT_PORT;
			} else if (!stricmp(protocol, "ipp")) {
				port = 631;
			} else {
				port = DEFAULT_PORT;
			}
		}
		__sock = new Socket(url.Host(), port);
		if (__sock->fail()) {
			__error_msg = __sock->getLastError();
		} else {
			connected = true;
		}
	}
}

const char *HttpURLConnection::getContentType()
{
	return getHeaderField("Content-Type");
}

const char *HttpURLConnection::getContentEncoding()
{
	return getHeaderField("Content-Encoding");
}

int HttpURLConnection::getContentLength()
{
	const char *p = getHeaderField("Content-Length");
	return p ? atoi(p) : -1;
}

const char *HttpURLConnection::getHeaderField(const char *s)
{
	if (__response == NULL) {
		action();
	}
	if (__response) {
		for (Fields::iterator it = __response->begin(); it != __response->end(); it++) {
			if ((*it).key == s) {
				return (*it).value.c_str();
			}
		}
	}
	return NULL;
}

HTTP_RESPONSECODE HttpURLConnection::getResponseCode()
{
	if (__response == NULL) {
		action();
	}
	return __response_code;
}

const char *HttpURLConnection::getResponseMessage()
{
	if (__response == NULL) {
		action();
	}
	return __response_message.c_str();
}

void HttpURLConnection::action()
{
	if (!connected) {
		connect();
	}
	if (connected) {
		setRequest();
	}
	if (connected) {
		getResponse();
	}
}

void HttpURLConnection::setRequest()
{
	if (connected) {
		setRequestProperty("Host", url.Host());
		ostream &os = getOutputStream();
		os << __method << ' ' << url.Path() << " HTTP/1.1" << '\r' << '\n';
		for (Fields::iterator it = __request->begin(); it != __request->end(); it++) {
			os << (*it).key << ": " << (*it).value << '\r' << '\n';
		}
		os << '\r' << '\n';

		setContent();

		if (!doOutput) {
			os.flush();
		}

		if (__response) {
			delete __response;
			__response = NULL;
		}
	}
}

void HttpURLConnection::setContent()
{
}

void HttpURLConnection::getResponse()
{
	if (connected) {

		if (__response == NULL) {
			__response = new Fields;

			istream &is = getInputStream();

			char buffer[1024];

			if (!is.getline(buffer, sizeof(buffer))) {
				__error_msg = __sock->getLastError();
				return;
			}
			buffer[is.gcount() - 2] = '\0';
			__response_message = buffer;
			strtok(buffer, " ");
			char *p = strtok(NULL, " ");
			__response_code = p ? (HTTP_RESPONSECODE)atoi(p) : HTTP_UNKNOWN;

			while (is.getline(buffer, sizeof(buffer))) {
				if (buffer[0] == '\r') {
					break;
				}
				buffer[is.gcount() - 2] = '\0';
				__response->push_back(Field(buffer));
			}

			int size = getContentLength();
			if (size > 0) {
				getContent();
			}

			if (__response_code != HTTP_CONTINUE) {
				const char *s = getHeaderField("Connection");
				if (s == NULL) {
					connected = false;
					__error_msg = "cannot found \"Connection\" field";
				} else if (stricmp(s, "Keep-Alive")) {
					connected = false;
				}
			}

			switch (__response_code) {
			case HTTP_MOVED_TEMP:
				{
					const char *p = getHeaderField("Location");
					if (p) {
						BUrl trueUrl(p);
						url = trueUrl;
						delete __response;
						__response = NULL;
						action();
					}
				}
				break;
			case HTTP_CONTINUE:
				delete __response;
				__response = NULL;
				getResponse();
				break;
			default:
				break;
			}
		}
	}
}

void HttpURLConnection::getContent()
{
	const int maxBufSize = 1024;
	if (connected) {
		int size = getContentLength();
		if (size > 0) {
			istream &is = getInputStream();
			int bufsize = min(size, maxBufSize);
			char buf[maxBufSize];
			while (size > 0 && is.read(buf, bufsize)) {
				size -= bufsize;
			}
		}
	}
}
