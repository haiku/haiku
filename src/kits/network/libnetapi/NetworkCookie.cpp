/*
 * Copyright 2010-2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Hamish Morrison, hamishm53@gmail.com
 */


#include <new>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <Debug.h>
#include <HttpTime.h>
#include <NetworkCookie.h>

using BPrivate::BHttpTime;

static const char* kArchivedCookieName = "be:cookie.name";
static const char* kArchivedCookieValue = "be:cookie.value";
static const char* kArchivedCookieDomain = "be:cookie.domain";
static const char* kArchivedCookiePath = "be:cookie.path";
static const char* kArchivedCookieExpirationDate = "be:cookie.expirationdate";
static const char* kArchivedCookieSecure = "be:cookie.secure";
static const char* kArchivedCookieHttpOnly = "be:cookie.httponly";
static const char* kArchivedCookieHostOnly = "be:cookie.hostonly";


BNetworkCookie::BNetworkCookie(const char* name, const char* value)
{
	_Reset();
	fName = name;
	fValue = value;
}


BNetworkCookie::BNetworkCookie(const BString& cookieString)
{
	_Reset();
	ParseCookieString(cookieString);
}


BNetworkCookie::BNetworkCookie(const BString& cookieString,
	const BUrl& url)
{
	_Reset();
	ParseCookieStringFromUrl(cookieString, url);
}


BNetworkCookie::BNetworkCookie(BMessage* archive)
{
	_Reset();

	archive->FindString(kArchivedCookieName, &fName);
	archive->FindString(kArchivedCookieValue, &fValue);

	archive->FindString(kArchivedCookieDomain, &fDomain);
	archive->FindString(kArchivedCookiePath, &fPath);
	archive->FindBool(kArchivedCookieSecure, &fSecure);
	archive->FindBool(kArchivedCookieHttpOnly, &fHttpOnly);
	archive->FindBool(kArchivedCookieHostOnly, &fHostOnly);

	int32 expiration;
	if (archive->FindInt32(kArchivedCookieExpirationDate, &expiration)
			== B_OK) {
		SetExpirationDate((time_t)expiration);
	}
}


BNetworkCookie::BNetworkCookie()
{
	_Reset();
}


BNetworkCookie::~BNetworkCookie()
{
}


// #pragma mark String to cookie fields


BNetworkCookie&
BNetworkCookie::ParseCookieStringFromUrl(const BString& string,
	const BUrl& url)
{
	_Reset();

	BString name;
	BString value;
	int32 index = 0;

	// Parse the name and value of the cookie
	index = _ExtractNameValuePair(string, name, value, index);
	if (index == -1) {
		// The set-cookie-string is not valid
		return *this;
	}

	SetName(name);
	SetValue(value);

	// Parse the remaining cookie attributes.
	while (index < string.Length()) {
		ASSERT(string[index] == ';');
		index++;

		index = _ExtractAttributeValuePair(string, name, value, index);

		if (name.ICompare("secure") == 0)
			SetSecure(true);
		else if (name.ICompare("httponly") == 0)
			SetHttpOnly(true);

		// The following attributes require a value.
		if (value.IsEmpty())
			continue;

		if (name.ICompare("max-age") == 0) {
			// Validate the max-age value.
			char* end = NULL;
			long maxAge = strtol(value.String(), &end, 10);
			if (*end == '\0')
				SetMaxAge((int)maxAge);
		} else if (name.ICompare("expires") == 0) {
			BHttpTime date(value);
			SetExpirationDate(date.Parse());
		} else if (name.ICompare("domain") == 0) {
			SetDomain(value);
		} else if (name.ICompare("path") == 0) {
			SetPath(value);
		}
	}

	// If no domain was specified, we set a host-only domain from the URL.
	if (!HasDomain()) {
		SetDomain(url.Host());
		fHostOnly = true;
	} else {
		// Otherwise the setting URL must domain-match the domain it set.
		if (!IsValidForDomain(url.Host())) {
			// Invalidate the cookie.
			_Reset();
			return *this;
		}
		// We should also reject cookies with domains that match public
		// suffixes.
	}

	// If no path was specified or the path is invalid, we compute the default
	// path from the URL.
	if (!HasPath() || Path()[0] != '/')
		SetPath(_DefaultPathForUrl(url));

	return *this;
}


BNetworkCookie&
BNetworkCookie::ParseCookieString(const BString& string)
{
	BUrl url;
	ParseCookieStringFromUrl(string, url);
	return *this;
}


// #pragma mark Cookie fields modification


BNetworkCookie&
BNetworkCookie::SetName(const BString& name)
{
	fName = name;
	fRawFullCookieValid = false;
	fRawCookieValid = false;
	return *this;
}


BNetworkCookie&
BNetworkCookie::SetValue(const BString& value)
{
	fValue = value;
	fRawFullCookieValid = false;
	fRawCookieValid = false;
	return *this;
}


BNetworkCookie&
BNetworkCookie::SetPath(const BString& path)
{
	// TODO: canonicalize the path
	fPath = path;
	fRawFullCookieValid = false;
	return *this;
}


BNetworkCookie&
BNetworkCookie::SetDomain(const BString& domain)
{
	// TODO: canonicalize the domain
	fDomain = domain;
	fHostOnly = false;

	fRawFullCookieValid = false;
	return *this;
}


BNetworkCookie&
BNetworkCookie::SetMaxAge(int32 maxAge)
{
	BDateTime expiration = BDateTime::CurrentDateTime(B_GMT_TIME);
	expiration.Time().AddSeconds(maxAge);
	return SetExpirationDate(expiration);
}


BNetworkCookie&
BNetworkCookie::SetExpirationDate(time_t expireDate)
{
	BDateTime expiration;
	expiration.SetTime_t(expireDate);
	return SetExpirationDate(expiration);
}


BNetworkCookie&
BNetworkCookie::SetExpirationDate(BDateTime& expireDate)
{
	if (expireDate.Time_t() <= 0) {
		fExpiration.SetTime_t(0);
		fSessionCookie = true;
		fExpirationStringValid = false;
		fRawFullCookieValid = false;
	} else {
		fExpiration = expireDate;
		fSessionCookie = false;
		fExpirationStringValid = false;
		fRawFullCookieValid = false;
	}

	return *this;
}


BNetworkCookie&
BNetworkCookie::SetSecure(bool secure)
{
	fSecure = secure;
	fRawFullCookieValid = false;
	return *this;
}


BNetworkCookie&
BNetworkCookie::SetHttpOnly(bool httpOnly)
{
	fHttpOnly = httpOnly;
	fRawFullCookieValid = false;
	return *this;
}


// #pragma mark Cookie fields access


const BString&
BNetworkCookie::Name() const
{
	return fName;
}


const BString&
BNetworkCookie::Value() const
{
	return fValue;
}


const BString&
BNetworkCookie::Domain() const
{
	return fDomain;
}


const BString&
BNetworkCookie::Path() const
{
	return fPath;
}


time_t
BNetworkCookie::ExpirationDate() const
{
	return fExpiration.Time_t();
}


const BString&
BNetworkCookie::ExpirationString() const
{
	BHttpTime date(ExpirationDate());

	if (!fExpirationStringValid) {
		fExpirationString = date.ToString(BPrivate::B_HTTP_TIME_FORMAT_COOKIE);
		fExpirationStringValid = true;
	}

	return fExpirationString;
}


bool
BNetworkCookie::Secure() const
{
	return fSecure;
}


bool
BNetworkCookie::HttpOnly() const
{
	return fHttpOnly;
}


const BString&
BNetworkCookie::RawCookie(bool full) const
{
	if (full && !fRawFullCookieValid) {
		fRawFullCookie.Truncate(0);
		fRawFullCookieValid = true;

		fRawFullCookie << fName << "=" << fValue;

		if (HasDomain())
			fRawFullCookie << "; Domain=" << fDomain;
		if (HasExpirationDate())
			fRawFullCookie << "; Expires=" << ExpirationString();
		if (HasPath())
			fRawFullCookie << "; Path=" << fPath;
		if (Secure())
			fRawFullCookie << "; Secure";
		if (HttpOnly())
			fRawFullCookie << "; HttpOnly";

	} else if (!full && !fRawCookieValid) {
		fRawCookie.Truncate(0);
		fRawCookieValid = true;

		fRawCookie << fName << "=" << fValue;
	}

	return full ? fRawFullCookie : fRawCookie;
}


// #pragma mark Cookie test


bool
BNetworkCookie::IsHostOnly() const
{
	return fHostOnly;
}


bool
BNetworkCookie::IsSessionCookie() const
{
	return fSessionCookie;
}


bool
BNetworkCookie::IsValid() const
{
	return HasName() && HasDomain() && HasPath();
}


bool
BNetworkCookie::IsValidForUrl(const BUrl& url) const
{
	if (Secure() && url.Protocol() != "https")
		return false;

	return IsValidForDomain(url.Host()) && IsValidForPath(url.Path());
}


bool
BNetworkCookie::IsValidForDomain(const BString& domain) const
{
	// TODO: canonicalize both domains
	const BString& cookieDomain = Domain();

	int32 difference = domain.Length() - cookieDomain.Length();
	// If the cookie domain is longer than the domain string it cannot
	// be valid.
	if (difference < 0)
		return false;

	// If the cookie is host-only the domains must match exactly.
	if (IsHostOnly())
		return domain == cookieDomain;

	// Otherwise, the domains must match exactly, or the cookie domain
	// must be a suffix with the preceeding character being a dot.
	const char* suffix = domain.String() + difference;
	if (strcmp(suffix, cookieDomain.String()) == 0) {
		if (difference == 0)
			return true;
		else if (domain[difference - 1] == '.')
			return true;
	}

	return false;
}


bool
BNetworkCookie::IsValidForPath(const BString& path) const
{
	const BString& cookiePath = Path();
	if (path.Length() < cookiePath.Length())
		return false;

	// The cookie path must be a prefix of the path string
	if (path.Compare(cookiePath, cookiePath.Length()) != 0)
		return false;

	// The paths match if they are identical, or if the last
	// character of the prefix is a slash, or if the character
	// after the prefix is a slash.
	if (path.Length() == cookiePath.Length()
			|| cookiePath[cookiePath.Length() - 1] == '/'
			|| path[cookiePath.Length()] == '/')
		return true;

	return false;
}


// #pragma mark Cookie fields existence tests


bool
BNetworkCookie::HasName() const
{
	return fName.Length() > 0;
}


bool
BNetworkCookie::HasValue() const
{
	return fValue.Length() > 0;
}


bool
BNetworkCookie::HasDomain() const
{
	return fDomain.Length() > 0;
}


bool
BNetworkCookie::HasPath() const
{
	return fPath.Length() > 0;
}


bool
BNetworkCookie::HasExpirationDate() const
{
	return !IsSessionCookie();
}


// #pragma mark Cookie delete test


bool
BNetworkCookie::ShouldDeleteAtExit() const
{
	return IsSessionCookie() || ShouldDeleteNow();
}


bool
BNetworkCookie::ShouldDeleteNow() const
{
	if (HasExpirationDate())
		return (BDateTime::CurrentDateTime(B_GMT_TIME) > fExpiration);

	return false;
}


// #pragma mark BArchivable members


status_t
BNetworkCookie::Archive(BMessage* into, bool deep) const
{
	status_t error = BArchivable::Archive(into, deep);

	if (error != B_OK)
		return error;

	error = into->AddString(kArchivedCookieName, fName);
	if (error != B_OK)
		return error;

	error = into->AddString(kArchivedCookieValue, fValue);
	if (error != B_OK)
		return error;


	// We add optional fields only if they're defined
	if (HasDomain()) {
		error = into->AddString(kArchivedCookieDomain, fDomain);
		if (error != B_OK)
			return error;
	}

	if (HasExpirationDate()) {
		error = into->AddInt32(kArchivedCookieExpirationDate,
			fExpiration.Time_t());
		if (error != B_OK)
			return error;
	}

	if (HasPath()) {
		error = into->AddString(kArchivedCookiePath, fPath);
		if (error != B_OK)
			return error;
	}

	if (Secure()) {
		error = into->AddBool(kArchivedCookieSecure, fSecure);
		if (error != B_OK)
			return error;
	}

	if (HttpOnly()) {
		error = into->AddBool(kArchivedCookieHttpOnly, fHttpOnly);
		if (error != B_OK)
			return error;
	}

	if (IsHostOnly()) {
		error = into->AddBool(kArchivedCookieHostOnly, true);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


/*static*/ BArchivable*
BNetworkCookie::Instantiate(BMessage* archive)
{
	if (archive->HasString(kArchivedCookieName)
		&& archive->HasString(kArchivedCookieValue))
		return new(std::nothrow) BNetworkCookie(archive);

	return NULL;
}


// #pragma mark Overloaded operators


BNetworkCookie&
BNetworkCookie::operator=(const char* string)
{
	return ParseCookieString(string);
}


bool
BNetworkCookie::operator==(const BNetworkCookie& other)
{
	// Equality : name and values equals
	return fName == other.fName && fValue == other.fValue;
}


bool
BNetworkCookie::operator!=(const BNetworkCookie& other)
{
	return !(*this == other);
}


void
BNetworkCookie::_Reset()
{
	fName.Truncate(0);
	fValue.Truncate(0);
	fDomain.Truncate(0);
	fPath.Truncate(0);
	fExpiration = BDateTime();
	fSecure = false;
	fHttpOnly = false;

	fSessionCookie = true;
	fHostOnly = true;

	fRawCookieValid = false;
	fRawFullCookieValid = false;
	fExpirationStringValid = false;
}


int32
skip_whitespace_forward(const BString& string, int32 index)
{
	while (index < string.Length() && (string[index] == ' '
			|| string[index] == '\t'))
		index++;
	return index;
}


int32
skip_whitespace_backward(const BString& string, int32 index)
{
	while (index >= 0 && (string[index] == ' ' || string[index] == '\t'))
		index--;
	return index;
}


int32
BNetworkCookie::_ExtractNameValuePair(const BString& cookieString,
	BString& name, BString& value, int32 index)
{
	// Find our name-value-pair and the delimiter.
	int32 firstEquals = cookieString.FindFirst('=', index);
	int32 nameValueEnd = cookieString.FindFirst(';', index);

	// If the set-cookie-string lacks a semicolon, the name-value-pair
	// is the whole string.
	if (nameValueEnd == -1)
		nameValueEnd = cookieString.Length();

	// If the name-value-pair lacks an equals, the parse should fail.
	if (firstEquals == -1 || firstEquals > nameValueEnd)
		return -1;

	int32 first = skip_whitespace_forward(cookieString, index);
	int32 last = skip_whitespace_backward(cookieString, firstEquals - 1);

	// If we lack a name, fail to parse.
	if (first > last)
		return -1;

	cookieString.CopyInto(name, first, last - first + 1);

	first = skip_whitespace_forward(cookieString, firstEquals + 1);
	last = skip_whitespace_backward(cookieString, nameValueEnd - 1);
	if (first <= last)
		cookieString.CopyInto(value, first, last - first + 1);
	else
		value.SetTo("");

	return nameValueEnd;
}


int32
BNetworkCookie::_ExtractAttributeValuePair(const BString& cookieString,
	BString& attribute, BString& value, int32 index)
{
	// Find the end of our cookie-av.
	int32 cookieAVEnd = cookieString.FindFirst(';', index);

	// If the unparsed-attributes lacks a semicolon, then the cookie-av is the
	// whole string.
	if (cookieAVEnd == -1)
		cookieAVEnd = cookieString.Length();

	int32 attributeNameEnd = cookieString.FindFirst('=', index);
	// If the cookie-av has no equals, the attribute-name is the entire
	// cookie-av and the attribute-value is empty.
	if (attributeNameEnd == -1 || attributeNameEnd > cookieAVEnd)
		attributeNameEnd = cookieAVEnd;

	int32 first = skip_whitespace_forward(cookieString, index);
	int32 last = skip_whitespace_backward(cookieString, attributeNameEnd - 1);

	if (first <= last)
		cookieString.CopyInto(attribute, first, last - first + 1);
	else
		attribute.SetTo("");

	if (attributeNameEnd == cookieAVEnd) {
		value.SetTo("");
		return cookieAVEnd;
	}

	first = skip_whitespace_forward(cookieString, attributeNameEnd + 1);
	last = skip_whitespace_backward(cookieString, cookieAVEnd - 1);
	if (first <= last)
		cookieString.CopyInto(value, first, last - first + 1);
	else
		value.SetTo("");

	return cookieAVEnd;
}


BString
BNetworkCookie::_DefaultPathForUrl(const BUrl& url)
{
	const BString& path = url.Path();
	if (path.IsEmpty() || path.ByteAt(0) != '/')
		return "";

	int32 index = path.FindLast('/');
	if (index == 0)
		return "";

	BString newPath = path;
	newPath.Truncate(index);
	return newPath;
}
