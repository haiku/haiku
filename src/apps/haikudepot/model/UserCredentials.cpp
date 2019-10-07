/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "UserCredentials.h"


// These are keys that are used to store this object's data into a BMessage
// instance.

#define KEY_NICKNAME		"nickname"
#define KEY_PASSWORD_CLEAR	"passwordClear"
#define KEY_IS_SUCCESSFUL	"isSuccessful"


UserCredentials::UserCredentials(BMessage* from)
{
	from->FindString(KEY_NICKNAME, &fNickname);
	from->FindString(KEY_PASSWORD_CLEAR, &fPasswordClear);
	from->FindBool(KEY_IS_SUCCESSFUL, &fIsSuccessful);
}


UserCredentials::UserCredentials(const BString& nickname,
	const BString& passwordClear)
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


UserCredentials&
UserCredentials::operator=(const UserCredentials& other)
{
	fNickname = other.fNickname;
	fPasswordClear = other.fPasswordClear;
	fIsSuccessful = other.fIsSuccessful;
	return *this;
}


status_t
UserCredentials::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	if (result == B_OK)
		result = into->AddString(KEY_NICKNAME, fNickname);
	if (result == B_OK)
		result = into->AddString(KEY_PASSWORD_CLEAR, fPasswordClear);
	if (result == B_OK)
		result = into->AddBool(KEY_IS_SUCCESSFUL, fIsSuccessful);
	return result;
}