/*
 * Copyright 2019-2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "CreateUserDetail.h"

// These are keys that are used to store this object's data into a BMessage
// instance.

static const char* const kKeyNickname = "nickname";
static const char* const kKeyPasswordClear = "password_clear";
static const char* const kKeyIsPasswordRepeated = "is_password_repeated";
static const char* const kKeyEmail = "email";
static const char* const kKeyCaptchaToken = "captcha_token";
static const char* const kKeyCaptchaResponse = "captcha_response";
static const char* const kKeyLanguageId = "language_id";
static const char* const kKeyAgreedUserUsageConditionsCode = "agreed_user_usage_conditions_code";


CreateUserDetail::CreateUserDetail(BMessage* from)
{
	from->FindString(kKeyNickname, &fNickname);
	from->FindString(kKeyPasswordClear, &fPasswordClear);
	from->FindBool(kKeyIsPasswordRepeated, &fIsPasswordRepeated);
	from->FindString(kKeyEmail, &fEmail);
	from->FindString(kKeyCaptchaToken, &fCaptchaToken);
	from->FindString(kKeyCaptchaResponse, &fCaptchaResponse);
	from->FindString(kKeyLanguageId, &fLanguageId);
	from->FindString(kKeyAgreedUserUsageConditionsCode, &fAgreedUserUsageConditionsCode);
}


CreateUserDetail::CreateUserDetail()
	:
	fIsPasswordRepeated(false)
{
}


CreateUserDetail::~CreateUserDetail()
{
}


const BString&
CreateUserDetail::Nickname() const
{
	return fNickname;
}


const BString&
CreateUserDetail::PasswordClear() const
{
	return fPasswordClear;
}


bool
CreateUserDetail::IsPasswordRepeated() const
{
	return fIsPasswordRepeated;
}


const BString&
CreateUserDetail::Email() const
{
	return fEmail;
}


const BString&
CreateUserDetail::CaptchaToken() const
{
	return fCaptchaToken;
}


const BString&
CreateUserDetail::CaptchaResponse() const
{
	return fCaptchaResponse;
}


const BString&
CreateUserDetail::LanguageId() const
{
	return fLanguageId;
}


const BString&
CreateUserDetail::AgreedToUserUsageConditionsCode() const
{
	return fAgreedUserUsageConditionsCode;
}


void
CreateUserDetail::SetNickname(const BString& value)
{
	fNickname = value;
}


void
CreateUserDetail::SetPasswordClear(const BString& value)
{
	fPasswordClear = value;
}


void
CreateUserDetail::SetIsPasswordRepeated(bool value)
{
	fIsPasswordRepeated = value;
}


void
CreateUserDetail::SetEmail(const BString& value)
{
	fEmail = value;
}


void
CreateUserDetail::SetCaptchaToken(const BString& value)
{
	fCaptchaToken = value;
}


void
CreateUserDetail::SetCaptchaResponse(const BString& value)
{
	fCaptchaResponse = value;
}


void
CreateUserDetail::SetLanguageId(const BString& value)
{
	fLanguageId = value;
}


void
CreateUserDetail::SetAgreedToUserUsageConditionsCode(const BString& value)
{
	fAgreedUserUsageConditionsCode = value;
}


status_t
CreateUserDetail::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	if (result == B_OK)
		result = into->AddString(kKeyNickname, fNickname);
	if (result == B_OK)
		result = into->AddString(kKeyPasswordClear, fPasswordClear);
	if (result == B_OK)
		result = into->AddBool(kKeyIsPasswordRepeated, fIsPasswordRepeated);
	if (result == B_OK)
		result = into->AddString(kKeyEmail, fEmail);
	if (result == B_OK)
		result = into->AddString(kKeyCaptchaToken, fCaptchaToken);
	if (result == B_OK)
		result = into->AddString(kKeyCaptchaResponse, fCaptchaResponse);
	if (result == B_OK)
		result = into->AddString(kKeyLanguageId, fLanguageId);
	if (result == B_OK)
		result = into->AddString(kKeyAgreedUserUsageConditionsCode, fAgreedUserUsageConditionsCode);
	return result;
}