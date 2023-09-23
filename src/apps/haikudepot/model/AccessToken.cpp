/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "AccessToken.h"

#include "JwtTokenHelper.h"
#include "Logger.h"

// These are keys that are used to store this object's data into a BMessage instance.

#define KEY_TOKEN				"token"
#define KEY_EXPIRY_TIMESTAMP	"expiryTimestamp"


AccessToken::AccessToken(BMessage* from)
	:
	fToken(""),
	fExpiryTimestamp(0)
{
	if (from->FindString(KEY_TOKEN, &fToken) != B_OK) {
		HDERROR("expected key [%s] in the message data when creating an access"
			" token", KEY_TOKEN);
	}

	if (from->FindUInt64(KEY_EXPIRY_TIMESTAMP, &fExpiryTimestamp) != B_OK) {
		HDERROR("expected key [%s] in the message data when creating an access"
			" token", KEY_EXPIRY_TIMESTAMP);
	}
}


AccessToken::AccessToken()
	:
	fToken(""),
	fExpiryTimestamp(0)
{
}


AccessToken::~AccessToken()
{
}


AccessToken&
AccessToken::operator=(const AccessToken& other)
{
	fToken = other.fToken;
	fExpiryTimestamp = other.fExpiryTimestamp;
	return *this;
}


bool
AccessToken::operator==(const AccessToken& other) const
{
	return fToken == other.fToken;
}


bool
AccessToken::operator!=(const AccessToken& other) const
{
	return !(*this == other);
}


const BString&
AccessToken::Token() const
{
	return fToken;
}


uint64
AccessToken::ExpiryTimestamp() const
{
	return fExpiryTimestamp;
}


void
AccessToken::SetToken(const BString& value)
{
	fToken = value;
}


void
AccessToken::SetExpiryTimestamp(uint64 value)
{
	fExpiryTimestamp = value;
}


/*! The access token may have a value or may be the empty string. This method
    will check that the token appears to be an access token.
*/

bool
AccessToken::IsValid() const
{
	return JwtTokenHelper::IsValid(fToken);
}


bool
AccessToken::IsValid(uint64 currentTimestamp) const
{
	return IsValid() && (fExpiryTimestamp == 0 || fExpiryTimestamp > currentTimestamp);
}


void
AccessToken::Clear()
{
	fToken = "";
    fExpiryTimestamp = 0;
}


status_t
AccessToken::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	if (result == B_OK && into == NULL)
		result = B_ERROR;
	if (result == B_OK)
		result = into->AddString(KEY_TOKEN, fToken);
	if (result == B_OK)
		result = into->AddUInt64(KEY_EXPIRY_TIMESTAMP, fExpiryTimestamp);
	return result;
}