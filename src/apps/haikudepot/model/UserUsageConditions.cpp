/*
 * Copyright 2019-2026, Andrew Lindesay <apl@lindesay.co.nz>.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "UserUsageConditions.h"

#include "Logger.h"

// These are keys that are used to store this object's data into a BMessage
// instance.

static const char* const kKeyCode = "code";
static const char* const kKeyCopyMarkdown = "copy_markdown";
static const char* const kKeyMinimumAge = "minimum_age";


UserUsageConditions::UserUsageConditions(BMessage* from)
	:
	fCode(""),
	fCopyMarkdown(""),
	fMinimumAge(0)
{
	int16 minimumAge;

	if (from->FindInt16(kKeyMinimumAge, &minimumAge) != B_OK)
		HDERROR("expected key [%s] in the message data", kKeyMinimumAge);
	fMinimumAge = (uint8) minimumAge;

	if (from->FindString(kKeyCode, &fCode) != B_OK)
		HDERROR("expected key [%s] in the message data", kKeyCode);
	if (from->FindString(kKeyCopyMarkdown, &fCopyMarkdown) != B_OK)
		HDERROR("expected key [%s] in the message data", kKeyCopyMarkdown);
}


UserUsageConditions::UserUsageConditions()
	:
	fCode(""),
	fCopyMarkdown(""),
	fMinimumAge(0)
{
}


UserUsageConditions::~UserUsageConditions()
{
}


const BString&
UserUsageConditions::Code() const
{
	return fCode;
}


const uint8
UserUsageConditions::MinimumAge() const
{
	return fMinimumAge;
}


const BString&
UserUsageConditions::CopyMarkdown() const
{
	return fCopyMarkdown;
}


void
UserUsageConditions::SetCode(const BString& code)
{
	fCode = code;
}


void
UserUsageConditions::SetMinimumAge(uint8 age)
{
	fMinimumAge = age;
}


void
UserUsageConditions::SetCopyMarkdown(const BString& copyMarkdown)
{
	fCopyMarkdown = copyMarkdown;
}


status_t
UserUsageConditions::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	if (result == B_OK)
		result = into->AddInt16(kKeyMinimumAge, (int16)fMinimumAge);
	if (result == B_OK)
		result = into->AddString(kKeyCode, fCode);
	if (result == B_OK)
		result = into->AddString(kKeyCopyMarkdown, fCopyMarkdown);
	return result;
}