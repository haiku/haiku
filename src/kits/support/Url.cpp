/*
 * Copyright 2007-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 *		Jonas Sundström, jonas@kirilla.com
 */

/*! Url class for parsing an URL and opening it with its preferred handler. */


#include <Debug.h>
#include <MimeType.h>
#include <Roster.h>
#include <StorageDefs.h>

#include <Url.h>


namespace BPrivate {
namespace Support {

BUrl::BUrl(const char* url)
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
		//		BAlert ... Too long URL!
		fprintf(stderr, "URL too long");
		return B_NAME_TOO_LONG;
	}
	
	char* argv[] = {
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
	
	CopyInto(fProto, 0, v);
	CopyInto(left, v + 1, Length() - v);
	// TODO: RFC1738 says the // part should indicate the uri follows the
	// u:p@h:p/path convention, so it should be used to check for special cases.
	if (left.FindFirst("//") == 0)
		left.RemoveFirst("//");
	fFull = left;
	
	// path part
	// actually some apps handle file://[host]/path
	// but I have no idea what proto it implies...
	// or maybe it's just to emphasize on "localhost".
	v = left.FindFirst("/");
	if (v == 0 || fProto == "file") {
		fPath = left;
		return B_OK;
	}
	// some protos actually implies path if it's the only component
	if ((v < 0) && (fProto == "beshare" || fProto == "irc")) { 
		fPath = left;
		return B_OK;
	}
	
	if (v > -1) {
		left.MoveInto(fPath, v+1, left.Length()-v);
		left.Remove(v, 1);
	}

	// user:pass@host
	v = left.FindFirst("@");
	if (v > -1) {
		left.MoveInto(fUser, 0, v);
		left.Remove(0, 1);
		v = fUser.FindFirst(":");
		if (v > -1) {
			fUser.MoveInto(fPass, v, fUser.Length() - v);
			fPass.Remove(0, 1);
		}
	} else if (fProto == "finger") {
		// single component implies user
		// see also: http://www.subir.com/lynx/lynx_help/lynx_url_support.html
		fUser = left;
		return B_OK;
	}

	// host:port
	v = left.FindFirst(":");
	if (v > -1) {
		left.MoveInto(fPort, v + 1, left.Length() - v);
		left.Remove(v, 1);
	}

	// not much left...
	fHost = left;

	return B_OK;
}


BString
BUrl::_UrlMimeType() const
{
	BString mime;
	mime << "application/x-vnd.Be.URL." << fProto;

	return BString(mime);
}


bool
BUrl::HasHost() const
{
	return fHost.Length();
}


bool
BUrl::HasPort() const
{
	return fPort.Length();
}


bool
BUrl::HasUser() const
{
	return fUser.Length();
}


bool
BUrl::HasPass() const
{
	return fPass.Length();
}


bool
BUrl::HasPath() const
{
	return fPath.Length();
}


const BString&
BUrl::Proto() const
{
	return fProto;
}


const BString&
BUrl::Full() const
{
	// RFC1738's "sheme-part"
	return fFull;
}


const BString&
BUrl::Host() const
{
	return fHost;
}


const BString&
BUrl::Port() const
{
	return fPort;
}


const BString&
BUrl::User() const
{
	return fUser;
}


const BString&
BUrl::Pass() const
{
	return fPass;
}


const BString&
BUrl::Path() const
{
	return fPath;
}


} // namespace Support
} // namespace BPrivate

