/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_NETWORK_COOKIE_H_
#define _B_NETWORK_COOKIE_H_


#include <Archivable.h>
#include <DateTime.h>
#include <Message.h>
#include <String.h>
#include <Url.h>


class BNetworkCookie : public BArchivable {
public:
								BNetworkCookie(const char* name,
									const char* value);
								BNetworkCookie(const BNetworkCookie& other);
								BNetworkCookie(const BString& cookieString);
								BNetworkCookie(const BString& cookieString,
									const BUrl& url);
								BNetworkCookie(BMessage* archive);
								BNetworkCookie();
	virtual						~BNetworkCookie();
							
	// Parse a "SetCookie" string, or "name=value"
	
			BNetworkCookie&		ParseCookieStringFromUrl(const BString& string, 
									const BUrl& url);
			BNetworkCookie& 	ParseCookieString(const BString& cookieString);
	
	// Modify the cookie fields
			BNetworkCookie&		SetComment(const BString& comment);
			BNetworkCookie&		SetCommentUrl(const BString& commentUrl);
			BNetworkCookie&		SetDiscard(bool discard);
			BNetworkCookie&		SetDomain(const BString& domain);
			BNetworkCookie&		SetMaxAge(int32 maxAge);
			BNetworkCookie&		SetExpirationDate(time_t expireDate);
			BNetworkCookie&		SetExpirationDate(BDateTime& expireDate);
			BNetworkCookie&		SetPath(const BString& path);
			BNetworkCookie& 	SetSecure(bool secure);
			BNetworkCookie&		SetVersion(int8 version);
			BNetworkCookie&		SetName(const BString& name);
			BNetworkCookie&		SetValue(const BString& value);
			
	// Access the cookie fields
			const BString&		CommentUrl() const;
			const BString&		Comment() const;
			bool				Discard() const;
			const BString&		Domain() const;
			int32				MaxAge() const;
			time_t				ExpirationDate() const;
			const BString&		ExpirationString() const;
			const BString&		Path() const;
			bool				Secure() const;
			int8 				Version() const;
			const BString&		Name() const;
			const BString&		Value() const;
			const BString&		RawCookie(bool full) const;
			bool				IsSessionCookie() const;
			bool				IsValid(bool strict = false) const;
			bool				IsValidForUrl(const BUrl& url) const;
			bool				IsValidForDomain(const BString& domain) const;
			bool				IsValidForPath(const BString& path) const;
			
	// Test if cookie fields are defined
			bool				HasCommentUrl() const;
			bool				HasComment() const;
			bool				HasDiscard() const;
			bool				HasDomain() const;
			bool				HasMaxAge() const;
			bool				HasExpirationDate() const;
			bool				HasPath() const;
			bool				HasVersion() const;
			bool				HasName() const;
			bool				HasValue() const;
			
	// Test if cookie could be deleted
			bool				ShouldDeleteAtExit() const;
			bool				ShouldDeleteNow() const;
			
	// BArchivable members
	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* archive);
	
	// Overloaded operators
			BNetworkCookie&		operator=(const BNetworkCookie& other);
			BNetworkCookie&		operator=(const char* string);
			bool				operator==(const BNetworkCookie& other);
			bool				operator!=(const BNetworkCookie& other);
private:
			void				_Reset();
			void				_ExtractNameValuePair(
									const BString& cookieString, int16* index, 
									bool parseField = false);
			
private:
	mutable	BString				fRawCookie;
	mutable	bool				fRawCookieValid;
	mutable	BString				fRawFullCookie;
	mutable	bool				fRawFullCookieValid;
	
			BString				fComment;
			BString				fCommentUrl;
			bool				fDiscard;
			BString				fDomain;
			BDateTime			fExpiration;
	mutable	BString				fExpirationString;
	mutable	bool				fExpirationStringValid;
			BString				fPath;
			bool				fSecure;
			int8				fVersion;
			BString				fName;
			BString				fValue;
			
			bool				fHasDiscard;
			bool				fHasExpirationDate;
			bool				fSessionCookie;
			bool				fHasVersion;
};

#endif // _B_NETWORK_COOKIE_H_

