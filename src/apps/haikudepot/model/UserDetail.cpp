/*
 * Copyright 2019-2026, Andrew Lindesay <apl@lindesay.co.nz>.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "UserDetail.h"


// These are keys that are used to store this object's data into a BMessage
// instance.

static const char* const kKeyNickname = "nickname";
static const char* const kKeyAgreement = "agreement";
static const char* const kKeyIsLatest = "is_latest";
static const char* const kKeyCode = "code";
static const char* const kKeyTimestampAgreed = "timestamp_agreed";


UserUsageConditionsAgreement::UserUsageConditionsAgreement(BMessage* from)
{
	from->FindUInt64(kKeyTimestampAgreed, &fTimestampAgreed);
	from->FindString(kKeyCode, &fCode);
	from->FindBool(kKeyIsLatest, &fIsLatest);
}


UserUsageConditionsAgreement::UserUsageConditionsAgreement()
	:
	fCode(),
	fTimestampAgreed(0),
	fIsLatest(false)
{
}


UserUsageConditionsAgreement::~UserUsageConditionsAgreement()
{
}


const BString&
UserUsageConditionsAgreement::Code() const
{
	return fCode;
}


const uint64
UserUsageConditionsAgreement::TimestampAgreed() const
{
	return fTimestampAgreed;
}


const bool
UserUsageConditionsAgreement::IsLatest() const
{
	return fIsLatest;
}


void
UserUsageConditionsAgreement::SetCode(const BString& value)
{
	fCode = value;
}


void
UserUsageConditionsAgreement::SetTimestampAgreed(uint64 value)
{
	fTimestampAgreed = value;
}


void
UserUsageConditionsAgreement::SetIsLatest(const bool value)
{
	fIsLatest = value;
}


UserUsageConditionsAgreement&
UserUsageConditionsAgreement::operator=(
	const UserUsageConditionsAgreement& other)
{
	fCode = other.fCode;
	fTimestampAgreed = other.fTimestampAgreed;
	fIsLatest = other.fIsLatest;
	return *this;
}


status_t
UserUsageConditionsAgreement::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	if (result == B_OK)
		result = into->AddUInt64(kKeyTimestampAgreed, fTimestampAgreed);
	if (result == B_OK)
		result = into->AddString(kKeyCode, fCode);
	if (result == B_OK)
		result = into->AddBool(kKeyIsLatest, fIsLatest);
	return result;
}


UserDetail::UserDetail(BMessage* from)
{
	BMessage agreementMessage;
	if (from->FindMessage(kKeyAgreement, &agreementMessage) == B_OK)
		fAgreement = UserUsageConditionsAgreement(&agreementMessage);
	from->FindString(kKeyNickname, &fNickname);
}


UserDetail::UserDetail()
	:
	fNickname(),
	fAgreement()
{
}


UserDetail::~UserDetail()
{
}


const BString&
UserDetail::Nickname() const
{
	return fNickname;
}


const UserUsageConditionsAgreement&
UserDetail::Agreement() const
{
	return fAgreement;
}


void
UserDetail::SetNickname(const BString& value)
{
	fNickname = value;
}


void
UserDetail::SetAgreement(
	const UserUsageConditionsAgreement& value)
{
	fAgreement = value;
}


UserDetail&
UserDetail::operator=(const UserDetail& other)
{
	fNickname = other.fNickname;
	fAgreement = other.fAgreement;
	return *this;
}


status_t
UserDetail::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	if (result == B_OK) {
		BMessage agreementMessage;
		result = fAgreement.Archive(&agreementMessage, true);
		if (result == B_OK)
			result = into->AddMessage(kKeyAgreement, &agreementMessage);
	}
	if (result == B_OK)
		result = into->AddString(kKeyNickname, fNickname);
	return result;
}
