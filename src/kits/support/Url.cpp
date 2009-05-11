/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 *		Jonas Sundström, jonas@kirilla.com
 */

/*! Url class for parsing an URL and opening it with its preferred handler. */


#include "Url.h"

namespace BPrivate {
namespace Support {

BUrl::BUrl(const char *url)
	: BString(url)
{
	fStatus = ParseAndSplit();
}


BUrl::~BUrl()
{
}


status_t
BUrl::InitCheck() const
{
	return fStatus;
}


status_t
BUrl::ParseAndSplit()
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

}; // namespace Support
}; // namespace BPrivate

