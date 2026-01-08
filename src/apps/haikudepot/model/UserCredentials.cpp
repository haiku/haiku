/*
 * Copyright 2019-2026, Andrew Lindesay <apl@lindesay.co.nz>.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "UserCredentials.h"


// These are keys that are used to store this object's data into a BMessage
// instance.

static const char* const kKeyNickname = "nickname";
static const char* const kKeyPasswordClear = "password_clear";
static const char* const kKeyIsSuccessful = "is_successful";


UserCredentials::UserCredentials(BMessage* from)
{
	from->FindString(kKeyNickname, &fNickname);
	from->FindString(kKeyPasswordClear, &fPasswordClear);
	from->FindBool(kKeyIsSuccessful, &fIsSuccessful);
}


UserCredentials::UserCredentials(const BString& nickname, const BString& passwordClear)
	:
	fNickname(nickname),
	fPasswordClear(passwordClear),
	fIsSuccessful(false)
{
}


UserCredentials::UserCredentials(const UserCredentials& other)
	:
	fNickname(other.Nickname()),
	fPasswordClear(other.PasswordClear()),
	fIsSuccessful(false)
{
}


UserCredentials::UserCredentials()
	:
	fNickname(),
	fPasswordClear(),
	fIsSuccessful(false)
{
}


UserCredentials::~UserCredentials()
{
}


UserCredentials&
UserCredentials::operator=(const UserCredentials& other)
{
	fNickname = other.fNickname;
	fPasswordClear = other.fPasswordClear;
	fIsSuccessful = other.fIsSuccessful;
	return *this;
}


bool
UserCredentials::operator==(const UserCredentials& other) const
{
	return fNickname == other.fNickname && fPasswordClear == other.fPasswordClear
		&& fIsSuccessful == other.fIsSuccessful;
}


bool
UserCredentials::operator!=(const UserCredentials& other) const
{
	return !(*this == other);
}


const BString&
UserCredentials::Nickname() const
{
	return fNickname;
}


const BString&
UserCredentials::PasswordClear() const
{
	return fPasswordClear;
}


const bool
UserCredentials::IsSuccessful() const
{
	return fIsSuccessful;
}


const bool
UserCredentials::IsValid() const
{
	return !fNickname.IsEmpty() && !fPasswordClear.IsEmpty();
}


void
UserCredentials::SetNickname(const BString& value)
{
	fNickname = value;
}


void
UserCredentials::SetPasswordClear(const BString& value)
{
	fPasswordClear = value;
}


void
UserCredentials::SetIsSuccessful(bool value)
{
	fIsSuccessful = value;
}


status_t
UserCredentials::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	if (result == B_OK)
		result = into->AddString(kKeyNickname, fNickname);
	if (result == B_OK)
		result = into->AddString(kKeyPasswordClear, fPasswordClear);
	if (result == B_OK)
		result = into->AddBool(kKeyIsSuccessful, fIsSuccessful);
	return result;
}
