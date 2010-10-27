/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */


#include <cstdlib>
#include <ctime>
#include <new>

#include <HttpTime.h>
#include <NetworkCookie.h>

#include <cstdio>
#define PRINT(x) printf x;

using BPrivate::BHttpTime;

static const char* 	kArchivedCookieComment 			= "be:cookie.comment";
static const char* 	kArchivedCookieCommentUrl		= "be:cookie.commenturl";
static const char* 	kArchivedCookieDiscard			= "be:cookie.discard";
static const char* 	kArchivedCookieDomain 			= "be:cookie.domain";
static const char* 	kArchivedCookieExpirationDate	= "be:cookie.expiredate";
static const char* 	kArchivedCookiePath 			= "be:cookie.path";
static const char* 	kArchivedCookieSecure 			= "be:cookie.secure";
static const char* 	kArchivedCookieVersion 			= "be:cookie.version";
static const char* 	kArchivedCookieName 			= "be:cookie.name";
static const char* 	kArchivedCookieValue 			= "be:cookie.value";


BNetworkCookie::BNetworkCookie(const char* name, const char* value)
	: 
	fDiscard(false),
	fExpiration(BDateTime::CurrentDateTime(B_GMT_TIME)),
	fVersion(0),
	fName(name),
	fValue(value),
	fSessionCookie(true)
{
	_Reset();
}


BNetworkCookie::BNetworkCookie(const BNetworkCookie& other)
	: 
	BArchivable(),
	fDiscard(false),
	fExpiration(BDateTime::CurrentDateTime(B_GMT_TIME)),
	fVersion(0),
	fSessionCookie(true)
{
	_Reset();
	*this = other;
}


BNetworkCookie::BNetworkCookie(const BString& cookieString)
	: 
	fDiscard(false),
	fExpiration(BDateTime::CurrentDateTime(B_GMT_TIME)),
	fVersion(0),
	fSessionCookie(true)
{
	_Reset();
	ParseCookieString(cookieString);
}


BNetworkCookie::BNetworkCookie(const BString& cookieString,
	const BUrl& url)
	: 
	fDiscard(false),
	fExpiration(BDateTime::CurrentDateTime(B_GMT_TIME)),
	fVersion(0),
	fSessionCookie(true)
{
	_Reset();
	ParseCookieStringFromUrl(cookieString, url);
}


BNetworkCookie::BNetworkCookie(BMessage* archive)
	: 
	fDiscard(false),
	fExpiration(BDateTime::CurrentDateTime(B_GMT_TIME)),
	fVersion(0),
	fSessionCookie(true)
{
	_Reset();
	
	archive->FindString(kArchivedCookieName, &fName);
	archive->FindString(kArchivedCookieValue, &fValue);
	
	archive->FindString(kArchivedCookieComment, &fComment);
	archive->FindString(kArchivedCookieCommentUrl, &fCommentUrl);
	archive->FindString(kArchivedCookieDomain, &fDomain);
	archive->FindString(kArchivedCookiePath, &fPath);
	archive->FindBool(kArchivedCookieSecure, &fSecure);
	
	if (archive->FindBool(kArchivedCookieDiscard, &fDiscard) == B_OK)
		fHasDiscard = true;
	
	if (archive->FindInt8(kArchivedCookieVersion, &fVersion) == B_OK)
		fHasVersion = true;
	
	int32 expiration;
	if (archive->FindInt32(kArchivedCookieExpirationDate, &expiration)
			== B_OK) {
		SetExpirationDate((time_t)expiration);
	}
}


BNetworkCookie::BNetworkCookie()
	: 
	fDiscard(false),
	fExpiration(BDateTime::CurrentDateTime(B_GMT_TIME)),
	fPath("/"),
	fVersion(0),
	fSessionCookie(true)
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
	SetPath(url.Path());
	
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
BNetworkCookie::SetComment(const BString& comment)
{
	fComment = comment;
	fRawFullCookieValid = false;
	return *this;
}


BNetworkCookie&
BNetworkCookie::SetCommentUrl(const BString& commentUrl)
{
	fCommentUrl = commentUrl;
	fRawFullCookieValid = false;
	return *this;
}


BNetworkCookie&
BNetworkCookie::SetDiscard(bool discard)
{
	fDiscard = discard;
	fHasDiscard = true;
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
BNetworkCookie::SetPath(const BString& path)
{
	fPath = path;
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
BNetworkCookie::SetVersion(int8 version)
{
	fVersion = version;
	fHasVersion = true;
	fRawCookieValid = false;
	return *this;
}


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


// #pragma mark Cookie fields access


const BString&
BNetworkCookie::Comment() const
{
	return fComment;
}


const BString&
BNetworkCookie::CommentUrl() const
{
	return fCommentUrl;
}


bool
BNetworkCookie::Discard() const
{
	return fDiscard;
}


const BString&
BNetworkCookie::Domain() const
{
	return fDomain;
}


int32
BNetworkCookie::MaxAge() const
{
	return fExpiration.Time_t() - BDateTime::CurrentDateTime(B_GMT_TIME).Time_t();
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


const BString&
BNetworkCookie::Path() const
{
	return fPath;
}


bool
BNetworkCookie::Secure() const
{
	return fSecure;
}


int8
BNetworkCookie::Version() const
{
	return fVersion;
}


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
BNetworkCookie::RawCookie(bool full) const
{
	if (full && !fRawFullCookieValid) {
		fRawFullCookie.Truncate(0);
		fRawFullCookieValid = true;
		
		fRawFullCookie << fName << "=" << fValue;
		
		if (HasCommentUrl())
			fRawFullCookie << "; Comment-Url=" << fCommentUrl;
		if (HasComment())
			fRawFullCookie << "; Comment=" << fComment;
		if (HasDiscard())
			fRawFullCookie << "; Discard=" << (fDiscard?"true":"false");
		if (HasDomain())
			fRawFullCookie << "; Domain=" << fDomain;
		if (HasExpirationDate())
			fRawFullCookie << "; Max-Age=" << MaxAge();
//			fRawFullCookie << "; Expires=" << ExpirationString();
		if (HasPath())
			fRawFullCookie << "; Path=" << fPath;
		if (Secure() && fSecure)
			fRawFullCookie << "; Secure=" << (fSecure?"true":"false");
		if (HasVersion())
			fRawFullCookie << ", Version=" << fVersion;
			
	} else if (!full && !fRawCookieValid) {
		fRawCookie.Truncate(0);
		fRawCookieValid = true;
		
		fRawCookie << fName << "=" << fValue;
	}

	return full?fRawFullCookie:fRawCookie;
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
	return HasName() && HasValue() && (!strict || HasVersion());
}


bool
BNetworkCookie::IsValidForUrl(const BUrl& url) const
{
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
BNetworkCookie::HasCommentUrl() const
{
	return fCommentUrl.Length() > 0;
}


bool
BNetworkCookie::HasComment() const
{
	return fComment.Length() > 0;
}


bool
BNetworkCookie::HasDiscard() const
{
	return fHasDiscard;
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
BNetworkCookie::HasVersion() const
{
	return fHasVersion;
}


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
BNetworkCookie::HasExpirationDate() const
{
	return fHasExpirationDate;
}


// #pragma mark Cookie delete test


bool
BNetworkCookie::ShouldDeleteAtExit() const
{
	return (HasDiscard() && Discard())
		|| (!IsSessionCookie() && ShouldDeleteNow())
		|| IsSessionCookie();
}


bool
BNetworkCookie::ShouldDeleteNow() const
{
	if (!IsSessionCookie() && HasExpirationDate())
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
	if (HasComment()) {
		error = into->AddString(kArchivedCookieComment, fComment);
		if (error != B_OK)
			return error;
	}	
	
	if (HasCommentUrl()) {
		error = into->AddString(kArchivedCookieCommentUrl, fCommentUrl);
		if (error != B_OK)
			return error;
	}
	
	if (HasDiscard()) {
		error = into->AddBool(kArchivedCookieDiscard, fDiscard);
		if (error != B_OK)
			return error;
	}
	
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
	
	if (HasVersion()) {
		error = into->AddInt8(kArchivedCookieVersion, fVersion);
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
	
	fComment					= other.fComment;
	fCommentUrl					= other.fCommentUrl;
	fDiscard					= other.fDiscard;
	fDomain						= other.fDomain;
	fExpiration					= other.fExpiration;
	fPath						= other.fPath;
	fSecure						= other.fSecure;
	fVersion					= other.fVersion;
	fName						= other.fName;
	fValue						= other.fValue;
	
	fHasDiscard					= other.fHasDiscard;
	fHasExpirationDate			= other.fHasExpirationDate;
	fSessionCookie				= other.fSessionCookie;
	fHasVersion					= other.fHasVersion;
	
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
	fComment.Truncate(0);
	fCommentUrl.Truncate(0);
	fDomain.Truncate(0);
	fPath.Truncate(0);
	fName.Truncate(0);
	fValue.Truncate(0);
	fDiscard 				= false;
	fSecure 				= false;
	fVersion 				= 0;
	fExpiration 			= 0;

	fHasDiscard 			= false;
	fHasExpirationDate 		= false;
	fSessionCookie 			= true;
	fHasVersion 			= false;
	
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

	// Cookie comment
	if (name == "comment")
		SetComment(value);
	// Cookie comment URL
	else if (name == "comment-url")
		SetCommentUrl(value);
	// Cookie discard flag
	else if (name == "discard")
		SetDiscard(value.Length() == 0 || value.ToLower() == "true");
	// Cookie max-age
	else if (name == "maxage")
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
	// Cookie version
	else if (name == "version")
		SetVersion(atoi(value.String()));
}
