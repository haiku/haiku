// Sun, 18 Jun 2000
// Y.Takagi

#include <cstring>
#include <stdlib.h>
#include "URL.h"

URL::URL(const char *spec)
{
//	__protocol = "http";
//	__host     = "localhost";
//	__file     = "/";
	__port     = -1;

	if (spec) {
		char *temp_spec = new char[strlen(spec) + 1];
		strcpy(temp_spec, spec);
	
		char *p1;
		char *p2;
		char *p3;
		char *p4;

		p1 = strstr(temp_spec, "//");
		if (p1) {
			*p1 = '\0';
			p1 += 2;
			__protocol = temp_spec;
		} else {
			p1 = temp_spec;
		}

		p3 = strstr(p1, "/");
		if (p3) {
			p4 = strstr(p3, "#");
			if (p4) {
				__ref = p4 + 1;
				*p4 = '\0';
			}
			__file = p3;
			*p3 = '\0';
		} else {
			__file = "/";
		}

		p2 = strstr(p1, ":");
		if (p2) {
			__port = atoi(p2 + 1);
			*p2 = '\0';
		}

		__host = p1;
		delete [] temp_spec;
	}

//	if (__port == -1) {
//		if (__protocol == "http") {
//			__port = 80;
//		} else if (__protocol == "ipp") {
//			__port = 631;
//		}
//	}
}

URL::URL(const char *protocol, const char *host, int port, const char *file)
{
	__protocol = protocol;
	__host     = host;
	__file     = file;
	__port     = port;
}

URL::URL(const char *protocol, const char *host, const char *file)
{
	__protocol = protocol;
	__host     = host;
	__file     = file;
}

URL::URL(const URL &url)
{
	__protocol = url.__protocol;
	__host     = url.__host;
	__file     = url.__file;
	__ref      = url.__ref;
	__port     = url.__port;
}

URL &URL::operator = (const URL &url)
{
	__protocol = url.__protocol;
	__host     = url.__host;
	__file     = url.__file;
	__ref      = url.__ref;
	__port     = url.__port;
	return *this;
}

bool URL::operator == (const URL &url)
{
	return (__protocol == url.__protocol) && (__host == url.__host) && (__file == url.__file) &&
			(__ref == url.__ref) && (__port == url.__port);
}
