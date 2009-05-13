/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 *		Jonas Sundström, jonas@kirilla.com
 */

/*! Url class for parsing an URL and opening it with its preferred handler. */

#define DEBUG 0

#include <ctype.h>
#include <stdio.h>
#include <unistd.h>

#include <Debug.h>
#include <MimeType.h>
#include <Roster.h>
#include <StorageDefs.h>

#include <Url.h>

namespace BPrivate {
namespace Support {

BUrl::BUrl(const char *url)
	: BString(url)
{
	fStatus = _ParseAndSplit();
}


BUrl::~BUrl()
{
}


status_t
BUrl::InitCheck() const
{
	return fStatus;
}


bool
BUrl::HasPreferredApplication() const
{
	BString appSignature = PreferredApplication();
	BMimeType mime(appSignature.String());

	if (appSignature.IFindFirst("application/") == 0
		&& mime.IsValid())
		return true;

	return false;
}


BString
BUrl::PreferredApplication() const
{
	BString appSignature;
	BMimeType mime(_UrlMimeType().String());
	mime.GetPreferredApp(appSignature.LockBuffer(B_MIME_TYPE_LENGTH));
	appSignature.UnlockBuffer();

	return BString(appSignature);
}


status_t
BUrl::OpenWithPreferredApplication(bool onProblemAskUser) const
{
	status_t status = InitCheck();
	if (status != B_OK)
		return status;

	if (Length() > B_PATH_NAME_LENGTH) {
		// TODO: BAlert
		//	if (onProblemAskUser)
		//		Balert ... Too long URL!
		fprintf(stderr, "URL too long");
		return B_NAME_TOO_LONG;
	}
	
	char *argv[] = {
		const_cast<char*>("BUrlInvokedApplication"),
		const_cast<char*>(String()),
		NULL
	};

#if DEBUG
	if (HasPreferredApplication())
		printf("HasPreferredApplication() == true\n");
	else
		printf("HasPreferredApplication() == false\n");
#endif

	status = be_roster->Launch(_UrlMimeType().String(), 1, argv+1);
	if (status != B_OK) {
		fprintf(stderr, "Opening URL failed: %s\n", strerror(status));
	}

	return status;
}


status_t
BUrl::_ParseAndSplit()
{
	// proto:[//]user:pass@host:port/path

	int32 v;
	BString left;

	v = FindFirst(":");
	if (v < 0)
		return B_BAD_VALUE;
	
	// TODO: proto and host should be lowercased.
	// see http://en.wikipedia.org/wiki/URL_normalization
	
	CopyInto(proto, 0, v);
	CopyInto(left, v + 1, Length() - v);
	// TODO: RFC1738 says the // part should indicate the uri follows the u:p@h:p/path convention, so it should be used to check for special cases.
	if (left.FindFirst("//") == 0)
		left.RemoveFirst("//");
	full = left;
	
	// path part
	// actually some apps handle file://[host]/path
	// but I have no idea what proto it implies...
	// or maybe it's just to emphasize on "localhost".
	v = left.FindFirst("/");
	if (v == 0 || proto == "file") {
		path = left;
		return 0;
	}
	// some protos actually implies path if it's the only component
	if ((v < 0) && (proto == "beshare" || proto == "irc")) { 
		path = left;
		return 0;
	}
	
	if (v > -1) {
		left.MoveInto(path, v+1, left.Length()-v);
		left.Remove(v, 1);
	}

	// user:pass@host
	v = left.FindFirst("@");
	if (v > -1) {
		left.MoveInto(user, 0, v);
		left.Remove(0, 1);
		v = user.FindFirst(":");
		if (v > -1) {
			user.MoveInto(pass, v, user.Length() - v);
			pass.Remove(0, 1);
		}
	} else if (proto == "finger") {
		// single component implies user
		// see also: http://www.subir.com/lynx/lynx_help/lynx_url_support.html
		user = left;
		return 0;
	}

	// host:port
	v = left.FindFirst(":");
	if (v > -1) {
		left.MoveInto(port, v + 1, left.Length() - v);
		left.Remove(v, 1);
	}

	// not much left...
	host = left;

	return 0;
}


BString
BUrl::_UrlMimeType() const
{
	BString mime;
	mime << "application/x-vnd.Be.URL." << proto;

	return BString(mime);
}


bool
BUrl::HasHost() const
{
	return host.Length();
}


bool
BUrl::HasPort() const
{
	return port.Length();
}


bool
BUrl::HasUser() const
{
	return user.Length();
}


bool
BUrl::HasPass() const
{
	return pass.Length();
}


bool
BUrl::HasPath() const
{
	return path.Length();
}


BString
BUrl::Proto() const
{
	return BString(proto);
}


BString
BUrl::Full() const
{
	// RFC1738's "sheme-part"
	return BString(full);
}


BString
BUrl::Host() const
{
	return BString(host);
}


BString
BUrl::Port() const
{
	return BString(port);
}


BString
BUrl::User() const
{
	return BString(user);
}


BString
BUrl::Pass() const
{
	return BString(pass);
}


BString
BUrl::Path() const
{
	return BString(path);
}


status_t
BUrl::UnurlString(BString &string)
{
	// TODO: check for %00 and bail out!
	int32 length = string.Length();
	int i;
	for (i = 0; string[i] && i < length - 2; i++) {
		if (string[i] == '%' && isxdigit(string[i+1]) && isxdigit(string[i+2])) {
			int c;
			sscanf(string.String() + i + 1, "%02x", &c);
			string.Remove(i, 3);
			string.Insert((char)c, 1, i);
			length -= 2;
		}
	}
	
	return B_OK;
}

}; // namespace Support
}; // namespace BPrivate

