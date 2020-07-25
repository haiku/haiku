/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
 #include "CreateUserDetail.h"

 // These are keys that are used to store this object's data into a BMessage
 // instance.

#define KEY_NICKNAME							"nickname"
#define KEY_PASSWORD_CLEAR						"passwordClear"
#define KEY_IS_PASSWORD_REPEATED				"isPasswordRepeated"
#define KEY_EMAIL								"email"
#define KEY_CAPTCHA_TOKEN						"captchaToken"
#define KEY_CAPTCHA_RESPONSE					"captchaResponse"
#define KEY_LANGUAGE_CODE						"languageCode"
#define KEY_AGREED_USER_USAGE_CONDITIONS_CODE	"agreedUserUsageConditionsCode"


CreateUserDetail::CreateUserDetail(BMessage* from)
{
	from->FindString(KEY_NICKNAME, &fNickname);
	from->FindString(KEY_PASSWORD_CLEAR, &fPasswordClear);
	from->FindBool(KEY_IS_PASSWORD_REPEATED, &fIsPasswordRepeated);
	from->FindString(KEY_EMAIL, &fEmail);
	from->FindString(KEY_CAPTCHA_TOKEN, &fCaptchaToken);
	from->FindString(KEY_CAPTCHA_RESPONSE, &fCaptchaResponse);
	from->FindString(KEY_LANGUAGE_CODE, &fLanguageCode);
	from->FindString(KEY_AGREED_USER_USAGE_CONDITIONS_CODE,
		&fAgreedUserUsageConditionsCode);
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
CreateUserDetail::LanguageCode() const
{
	return fLanguageCode;
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
CreateUserDetail::SetLanguageCode(const BString& value)
{
	fLanguageCode = value;
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
		result = into->AddString(KEY_NICKNAME, fNickname);
	if (result == B_OK)
		result = into->AddString(KEY_PASSWORD_CLEAR, fPasswordClear);
	if (result == B_OK)
		result = into->AddBool(KEY_IS_PASSWORD_REPEATED, fIsPasswordRepeated);
	if (result == B_OK)
		result = into->AddString(KEY_EMAIL, fEmail);
	if (result == B_OK)
		result = into->AddString(KEY_CAPTCHA_TOKEN, fCaptchaToken);
	if (result == B_OK)
		result = into->AddString(KEY_CAPTCHA_RESPONSE, fCaptchaResponse);
	if (result == B_OK)
		result = into->AddString(KEY_LANGUAGE_CODE, fLanguageCode);
	if (result == B_OK) {
		result = into->AddString(KEY_AGREED_USER_USAGE_CONDITIONS_CODE,
			fAgreedUserUsageConditionsCode);
	}
	return result;
}