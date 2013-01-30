/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */


#include <new>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <HttpTime.h>
#include <NetworkCookie.h>

#define PRINT(x) printf x;

using BPrivate::BHttpTime;

static const char* kArchivedCookieName = "be:cookie.name";
static const char* kArchivedCookieValue = "be:cookie.value";
static const char* kArchivedCookieDomain = "be:cookie.domain";
static const char* kArchivedCookiePath = "be:cookie.path";
static const char* kArchivedCookieExpirationDate = "be:cookie.expirationdate";
static const char* kArchivedCookieSecure = "be:cookie.secure";
static const char* kArchivedCookieHttpOnly = "be:cookie.httponly";


BNetworkCookie::BNetworkCookie(const char* name, const char* value)
	:
	fName(name),
	fValue(value),
	fSessionCookie(true)
{
	_Reset();
}


BNetworkCookie::BNetworkCookie(const BNetworkCookie& other)
{
	_Reset();
	*this = other;
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
	:
	fSessionCookie(true)
{
	_Reset();

	archive->FindString(kArchivedCookieName, &fName);
	archive->FindString(kArchivedCookieValue, &fValue);

	archive->FindString(kArchivedCookieDomain, &fDomain);
	archive->FindString(kArchivedCookiePath, &fPath);
	archive->FindBool(kArchivedCookieSecure, &fSecure);
	archive->FindBool(kArchivedCookieHttpOnly, &fHttpOnly);

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
	BString cookieString(string);
	int16 index = 0;

	_Reset();

	// Default values from url
	SetDomain(url.Host());
	_SetDefaultPathForUrl(url);

	_ExtractNameValuePair(cookieString, &index);

	while (index < cookieString.Length())
		_ExtractNameValuePair(cookieString, &index, true);

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
	fPath = path;
	fRawFullCookieValid = false;
	return *this;
}


BNetworkCookie&
BNetworkCookie::SetDomain(const BString& domain)
{
	fDomain = domain;

	//  We always use pre-dotted domains for tail matching
	if (fDomain.ByteAt(0) != '.')
		fDomain.Prepend(".");

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
		fHasExpirationDate = false;
	} else {
		fExpiration = expireDate;
		fSessionCookie = false;
		fExpirationStringValid = false;
		fRawFullCookieValid = false;
		fHasExpirationDate = true;
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
BNetworkCookie::IsSessionCookie() const
{
	return fSessionCookie;
}


bool
BNetworkCookie::IsValid(bool strict) const
{
	return HasName() && HasValue();
}


bool
BNetworkCookie::IsValidForUrl(const BUrl& url) const
{
	// TODO: Take secure attribute into account
	BString urlHost = url.Host();
	BString urlPath = url.Path();

	return IsValidForDomain(urlHost) && IsValidForPath(urlPath);
}


bool
BNetworkCookie::IsValidForDomain(const BString& domain) const
{
	if (fDomain.Length() > domain.Length())
		return false;

	return domain.FindLast(fDomain) == (domain.Length() - fDomain.Length());
}


bool
BNetworkCookie::IsValidForPath(const BString& path) const
{
	if (fPath.Length() > path.Length())
		return false;

	return path.FindFirst(fPath) == 0;
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
	return fHasExpirationDate;
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

	if (fHasExpirationDate) {
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
BNetworkCookie::operator=(const BNetworkCookie& other)
{
	// Should we prefer to discard the cache ?
	fRawCookie					= other.fRawCookie;
	fRawCookieValid				= other.fRawCookieValid;
	fRawFullCookie				= other.fRawFullCookie;
	fRawFullCookieValid			= other.fRawFullCookieValid;
	fExpirationString			= other.fExpirationString;
	fExpirationStringValid		= other.fExpirationStringValid;

	fName						= other.fName;
	fValue						= other.fValue;
	fDomain						= other.fDomain;
	fPath						= other.fPath;
	fExpiration					= other.fExpiration;
	fSecure						= other.fSecure;
	fHttpOnly					= other.fHttpOnly;

	fHasExpirationDate			= other.fHasExpirationDate;
	fSessionCookie				= other.fSessionCookie;

	return *this;
}


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
	fDomain.Truncate(0);
	fPath.Truncate(0);
	fName.Truncate(0);
	fValue.Truncate(0);
	fSecure 				= false;
	fHttpOnly				= false;
	fExpiration 			= BDateTime();

	fHasExpirationDate 		= false;
	fSessionCookie 			= true;

	fRawCookieValid 		= false;
	fRawFullCookieValid 	= false;
	fExpirationStringValid 	= false;
}


void
BNetworkCookie::_ExtractNameValuePair(const BString& cookieString,
	int16* index, bool parseField)
{
	// Skip whitespaces
	while (cookieString.ByteAt(*index) == ' '
		&& *index < cookieString.Length())
		(*index)++;

	if (*index >= cookieString.Length())
		return;


	// Look for a name=value pair
	int16 firstSemiColon = cookieString.FindFirst(";", *index);
	int16 firstEqual = cookieString.FindFirst("=", *index);

	BString name;
	BString value;

	if (firstSemiColon == -1) {
		if (firstEqual != -1) {
			cookieString.CopyInto(name, *index, firstEqual - *index);
			cookieString.CopyInto(value, firstEqual + 1,
				cookieString.Length() - firstEqual - 1);
		} else
			cookieString.CopyInto(value, *index,
				cookieString.Length() - *index);

		*index = cookieString.Length() + 1;
	} else {
		if (firstEqual != -1 && firstEqual < firstSemiColon) {
			cookieString.CopyInto(name, *index, firstEqual - *index);
			cookieString.CopyInto(value, firstEqual + 1,
				firstSemiColon - firstEqual - 1);
		} else
			cookieString.CopyInto(value, *index, firstSemiColon - *index);

		*index = firstSemiColon + 1;
	}

	// Cookie name/value pair
	if (!parseField) {
		SetName(name);
		SetValue(value);
		return;
	}

	name.ToLower();
	name.Trim();
	value.Trim();

	// Cookie max-age
	if (name == "maxage")
		SetMaxAge(atoi(value.String()));
	// Cookie expiration date
	else if (name == "expires") {
		BHttpTime date(value);
		SetExpirationDate(date.Parse());
	// Cookie valid domain
	} else if (name == "domain")
		SetDomain(value);
	// Cookie valid path
	else if (name == "path")
		SetPath(value);
	// Cookie secure flag
	else if (name == "secure")
		SetSecure(value.Length() == 0 || value.ToLower() == "true");
}


void
BNetworkCookie::_SetDefaultPathForUrl(const BUrl& url)
{
	const BString& path = url.Path();
	if (path.IsEmpty() || path.ByteAt(0) != '/') {
		SetPath("/");
		return;
	}

	int32 index = path.FindLast('/');
	if (index == 0) {
		SetPath("/");
		return;
	}

	BString newPath = path;
	SetPath(newPath.Truncate(index));
}
