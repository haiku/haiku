/*
 * Copyright 2019-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "UserUsageConditions.h"

#include "Logger.h"

// These are keys that are used to store this object's data into a BMessage
// instance.

#define KEY_COPY_MARKDOWN	"copyMarkdown"
#define KEY_CODE			"code"
#define KEY_MINIMUM_AGE		"minimumAge"


UserUsageConditions::UserUsageConditions(BMessage* from)
	:
	fCode(""),
	fCopyMarkdown(""),
	fMinimumAge(0)
{
	int16 minimumAge;

	if (from->FindInt16(KEY_MINIMUM_AGE, &minimumAge) != B_OK)
		HDERROR("expected key [%s] in the message data", KEY_MINIMUM_AGE);
	fMinimumAge = (uint8) minimumAge;

	if (from->FindString(KEY_CODE, &fCode) != B_OK)
		HDERROR("expected key [%s] in the message data", KEY_CODE);
	if (from->FindString(KEY_COPY_MARKDOWN, &fCopyMarkdown) != B_OK)
		HDERROR("expected key [%s] in the message data", KEY_COPY_MARKDOWN);
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
	into->AddInt16(KEY_MINIMUM_AGE, (int16) fMinimumAge);
	into->AddString(KEY_CODE, fCode);
	into->AddString(KEY_COPY_MARKDOWN, fCopyMarkdown);
	return B_OK;
}