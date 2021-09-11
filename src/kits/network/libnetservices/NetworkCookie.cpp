/*
 * Copyright 2010-2014 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Hamish Morrison, hamishm53@gmail.com
 */


#include <new>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <Debug.h>
#include <HttpTime.h>
#include <NetworkCookie.h>

using namespace BPrivate::Network;


static const char* kArchivedCookieName = "be:cookie.name";
static const char* kArchivedCookieValue = "be:cookie.value";
static const char* kArchivedCookieDomain = "be:cookie.domain";
static const char* kArchivedCookiePath = "be:cookie.path";
static const char* kArchivedCookieExpirationDate = "be:cookie.expirationdate";
static const char* kArchivedCookieSecure = "be:cookie.secure";
static const char* kArchivedCookieHttpOnly = "be:cookie.httponly";
static const char* kArchivedCookieHostOnly = "be:cookie.hostonly";


BNetworkCookie::BNetworkCookie(const char* name, const char* value,
	const BUrl& url)
{
	_Reset();
	fName = name;
	fValue = value;

	SetDomain(url.Host());

	if (url.Protocol() == "file" && url.Host().Length() == 0) {
		SetDomain("localhost");
			// make sure cookies set from a file:// URL are stored somewhere.
	}

	SetPath(_DefaultPathForUrl(url));
}


BNetworkCookie::BNetworkCookie(const BString& cookieString, const BUrl& url)
{
	_Reset();
	fInitStatus = ParseCookieString(cookieString, url);
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

	// We store the expiration date as a string, which should not overflow.
	// But we still parse the old archive format, where an int32 was used.
	BString expirationString;
	int32 expiration;
	if (archive->FindString(kArchivedCookieExpirationDate, &expirationString)
			== B_OK) {
		BDateTime time = BHttpTime(expirationString).Parse();
		SetExpirationDate(time);
	} else if (archive->FindInt32(kArchivedCookieExpirationDate, &expiration)
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


status_t
BNetworkCookie::ParseCookieString(const BString& string, const BUrl& url)
{
	_Reset();

	// Set default values (these can be overriden later on)
	SetPath(_DefaultPathForUrl(url));
	SetDomain(url.Host());
	fHostOnly = true;
	if (url.Protocol() == "file" && url.Host().Length() == 0) {
		fDomain = "localhost";
			// make sure cookies set from a file:// URL are stored somewhere.
			// not going through SetDomain as it requires at least one '.'
			// in the domain (to avoid setting cookies on TLDs).
	}

	BString name;
	BString value;
	int32 index = 0;

	// Parse the name and value of the cookie
	index = _ExtractNameValuePair(string, name, value, index);
	if (index == -1 || value.Length() > 4096) {
		// The set-cookie-string is not valid
		return B_BAD_DATA;
	}

	SetName(name);
	SetValue(value);

	// Note on error handling: even if there are parse errors, we will continue
	// and try to parse as much from the cookie as we can.
	status_t result = B_OK;

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

		if (name.ICompare("max-age") == 0) {
			if (value.IsEmpty()) {
				result = B_BAD_VALUE;
				continue;
			}
			// Validate the max-age value.
			char* end = NULL;
			errno = 0;
			long maxAge = strtol(value.String(), &end, 10);
			if (*end == '\0')
				SetMaxAge((int)maxAge);
			else if (errno == ERANGE && maxAge == LONG_MAX)
				SetMaxAge(INT_MAX);
			else
				SetMaxAge(-1); // cookie will expire immediately
		} else if (name.ICompare("expires") == 0) {
			if (value.IsEmpty()) {
				// Will be a session cookie.
				continue;
			}
			BDateTime parsed = BHttpTime(value).Parse();
			SetExpirationDate(parsed);
		} else if (name.ICompare("domain") == 0) {
			if (value.IsEmpty()) {
				result = B_BAD_VALUE;
				continue;
			}

			status_t domainResult = SetDomain(value);
			// Do not reset the result to B_OK if something else already failed
			if (result == B_OK)
				result = domainResult;
		} else if (name.ICompare("path") == 0) {
			if (value.IsEmpty()) {
				result = B_BAD_VALUE;
				continue;
			}
			status_t pathResult = SetPath(value);
			if (result == B_OK)
				result = pathResult;
		}
	}

	if (!_CanBeSetFromUrl(url))
		result = B_NOT_ALLOWED;

	if (result != B_OK)
		_Reset();

	return result;
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


status_t
BNetworkCookie::SetPath(const BString& to)
{
	fPath.Truncate(0);
	fRawFullCookieValid = false;

	// Limit the path to 4096 characters to not let the cookie jar grow huge.
	if (to[0] != '/' || to.Length() > 4096)
		return B_BAD_DATA;

	// Check that there aren't any "." or ".." segments in the path.
	if (to.EndsWith("/.") || to.EndsWith("/.."))
		return B_BAD_DATA;
	if (to.FindFirst("/../") >= 0 || to.FindFirst("/./") >= 0)
		return B_BAD_DATA;

	fPath = to;
	return B_OK;
}


status_t
BNetworkCookie::SetDomain(const BString& domain)
{
	// TODO: canonicalize the domain
	BString newDomain = domain;

	// RFC 2109 (legacy) support: domain string may start with a dot,
	// meant to indicate the cookie should also be used for subdomains.
	// RFC 6265 makes all cookies work for subdomains, unless the domain is
	// not specified at all (in this case it has to exactly match the Url of
	// the page that set the cookie). In any case, we don't need to handle
	// dot-cookies specifically anymore, so just remove the extra dot.
	if (newDomain[0] == '.')
		newDomain.Remove(0, 1);

	// check we're not trying to set a cookie on a TLD or empty domain
	if (newDomain.FindLast('.') <= 0)
		return B_BAD_DATA;

	fDomain = newDomain.ToLower();

	fHostOnly = false;

	fRawFullCookieValid = false;
	return B_OK;
}


BNetworkCookie&
BNetworkCookie::SetMaxAge(int32 maxAge)
{
	BDateTime expiration = BDateTime::CurrentDateTime(B_LOCAL_TIME);

	// Compute the expiration date (watch out for overflows)
	int64_t date = expiration.Time_t();
	date += (int64_t)maxAge;
	if (date > INT_MAX)
		date = INT_MAX;

	expiration.SetTime_t(date);

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
	if (!expireDate.IsValid()) {
		fExpiration.SetTime_t(0);
		fSessionCookie = true;
	} else {
		fExpiration = expireDate;
		fSessionCookie = false;
	}

	fExpirationStringValid = false;
	fRawFullCookieValid = false;

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
	BHttpTime date(fExpiration);

	if (!fExpirationStringValid) {
		fExpirationString = date.ToString(B_HTTP_TIME_FORMAT_COOKIE);
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
	if (!fRawCookieValid) {
		fRawCookie.Truncate(0);
		fRawCookieValid = true;

		fRawCookie << fName << "=" << fValue;
	}

	if (!full)
		return fRawCookie;

	if (!fRawFullCookieValid) {
		fRawFullCookie = fRawCookie;
		fRawFullCookieValid = true;

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

	}

	return fRawFullCookie;
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
	return fInitStatus == B_OK && HasName() && HasDomain();
}


bool
BNetworkCookie::IsValidForUrl(const BUrl& url) const
{
	if (Secure() && url.Protocol() != "https")
		return false;

	if (url.Protocol() == "file")
		return Domain() == "localhost" && IsValidForPath(url.Path());

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

	// FIXME do not do substring matching on IP addresses. The RFCs disallow it.

	// Otherwise, the domains must match exactly, or the domain must have a dot
	// character just before the common suffix.
	const char* suffix = domain.String() + difference;
	return (strcmp(suffix, cookieDomain.String()) == 0 && (difference == 0
		|| domain[difference - 1] == '.'));
}


bool
BNetworkCookie::IsValidForPath(const BString& path) const
{
	const BString& cookiePath = Path();
	BString normalizedPath = path;
	int slashPos = normalizedPath.FindLast('/');
	if (slashPos != normalizedPath.Length() - 1)
		normalizedPath.Truncate(slashPos + 1);

	if (normalizedPath.Length() < cookiePath.Length())
		return false;

	// The cookie path must be a prefix of the path string
	return normalizedPath.Compare(cookiePath, cookiePath.Length()) == 0;
}


bool
BNetworkCookie::_CanBeSetFromUrl(const BUrl& url) const
{
	if (url.Protocol() == "file")
		return Domain() == "localhost" && _CanBeSetFromPath(url.Path());

	return _CanBeSetFromDomain(url.Host()) && _CanBeSetFromPath(url.Path());
}


bool
BNetworkCookie::_CanBeSetFromDomain(const BString& domain) const
{
	// TODO: canonicalize both domains
	const BString& cookieDomain = Domain();

	int32 difference = domain.Length() - cookieDomain.Length();
	if (difference < 0) {
		// Setting a cookie on a subdomain is allowed.
		const char* suffix = cookieDomain.String() + difference;
		return (strcmp(suffix, domain.String()) == 0 && (difference == 0
			|| cookieDomain[difference - 1] == '.'));
	}

	// If the cookie is host-only the domains must match exactly.
	if (IsHostOnly())
		return domain == cookieDomain;

	// FIXME prevent supercookies with a domain of ".com" or similar
	// This is NOT as straightforward as relying on the last dot in the domain.
	// Here's a list of TLD:
	// https://github.com/rsimoes/Mozilla-PublicSuffix/blob/master/effective_tld_names.dat

	// FIXME do not do substring matching on IP addresses. The RFCs disallow it.

	// Otherwise, the domains must match exactly, or the domain must have a dot
	// character just before the common suffix.
	const char* suffix = domain.String() + difference;
	return (strcmp(suffix, cookieDomain.String()) == 0 && (difference == 0
		|| domain[difference - 1] == '.'));
}


bool
BNetworkCookie::_CanBeSetFromPath(const BString& path) const
{
	BString normalizedPath = path;
	int slashPos = normalizedPath.FindLast('/');
	normalizedPath.Truncate(slashPos);

	if (Path().Compare(normalizedPath, normalizedPath.Length()) == 0)
		return true;
	else if (normalizedPath.Compare(Path(), Path().Length()) == 0)
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
		error = into->AddString(kArchivedCookieExpirationDate,
			BHttpTime(fExpiration).ToString());
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
	fInitStatus = false;

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

	// values may (or may not) have quotes around them.
	if (value[0] == '"' && value[value.Length() - 1] == '"') {
		value.Remove(0, 1);
		value.Remove(value.Length() - 1, 1);
	}

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
