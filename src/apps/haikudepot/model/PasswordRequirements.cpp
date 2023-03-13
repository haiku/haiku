/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PasswordRequirements.h"

// These are keys that are used to store this object's data into a BMessage
// instance.

#define KEY_MIN_PASSWORD_LENGTH			"minPasswordLength"
#define KEY_MIN_PASSWORD_UPPERCASE_CHAR	"minPasswordUppercaseChar"
#define KEY_MIN_PASSWORD_DIGITS_CHAR	"minPasswordDigitsChar"


PasswordRequirements::PasswordRequirements()
	:
	fMinPasswordLength(0),
	fMinPasswordUppercaseChar(0),
	fMinPasswordDigitsChar(0)
{
}


PasswordRequirements::PasswordRequirements(BMessage* from)
	:
	fMinPasswordLength(0),
	fMinPasswordUppercaseChar(0),
	fMinPasswordDigitsChar(0)
{
	uint32 value;

	if (from->FindUInt32(KEY_MIN_PASSWORD_LENGTH, &value) == B_OK) {
		fMinPasswordLength = value;
	}

	if (from->FindUInt32(
			KEY_MIN_PASSWORD_UPPERCASE_CHAR, &value) == B_OK) {
		fMinPasswordUppercaseChar = value;
	}

	if (from->FindUInt32(
			KEY_MIN_PASSWORD_DIGITS_CHAR, &value) == B_OK) {
		fMinPasswordDigitsChar = value;
	}
}


PasswordRequirements::~PasswordRequirements()
{
}


void
PasswordRequirements::SetMinPasswordLength(uint32 value)
{
	fMinPasswordLength = value;
}


void
PasswordRequirements::SetMinPasswordUppercaseChar(uint32 value)
{
	fMinPasswordUppercaseChar = value;
}


void
PasswordRequirements::SetMinPasswordDigitsChar(uint32 value)
{
	fMinPasswordDigitsChar = value;
}


status_t
PasswordRequirements::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	if (result == B_OK) {
		result = into->AddUInt32(
			KEY_MIN_PASSWORD_LENGTH, fMinPasswordLength);
	}
	if (result == B_OK) {
		result = into->AddUInt32(
			KEY_MIN_PASSWORD_UPPERCASE_CHAR, fMinPasswordUppercaseChar);
	}
	if (result == B_OK) {
		result = into->AddUInt32(
			KEY_MIN_PASSWORD_DIGITS_CHAR, fMinPasswordDigitsChar);
	}
	return result;
}
