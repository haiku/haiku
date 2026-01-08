/*
 * Copyright 2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "SimpleAlert.h"

#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SimpleAlert"


static const char* const kKeyAlertText = "text";
static const char* const kKeyAlertTitle = "title";
static const char* const kKeyAlertType = "type";


SimpleAlert::SimpleAlert()
	:
	fTitle(B_TRANSLATE("Error")),
	fText("?"),
	fType(B_INFO_ALERT)
{
}


SimpleAlert::SimpleAlert(const BMessage* from)
{
	if (from->FindString(kKeyAlertTitle, &fTitle) != B_OK)
		fTitle = B_TRANSLATE("Error");

	if (from->FindString(kKeyAlertText, &fText) != B_OK)
		fText = "?";

	uint32 typeInt;

	if (from->FindUInt32(kKeyAlertType, &typeInt) == B_OK)
		fType = static_cast<alert_type>(typeInt);
	else
		fType = B_INFO_ALERT;
}


SimpleAlert::SimpleAlert(const BString& title, const BString& text, alert_type type)
	:
	fTitle(title),
	fText(text),
	fType(type)
{
	if (fTitle.IsEmpty())
		fTitle = B_TRANSLATE("Error");
	if (fText.IsEmpty())
		fText = "?";
}


SimpleAlert::~SimpleAlert()
{
}


const BString
SimpleAlert::Title() const
{
	return fTitle;
}


const BString
SimpleAlert::Text() const
{
	return fText;
}


alert_type
SimpleAlert::Type() const
{
	return fType;
}


bool
SimpleAlert::operator==(const SimpleAlert& other) const
{
	return fTitle == other.fTitle && fText == other.fText && fType == other.fType;
}


status_t
SimpleAlert::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	if (result == B_OK)
		result = into->AddString(kKeyAlertTitle, fTitle);
	if (result == B_OK)
		result = into->AddString(kKeyAlertText, fText);
	if (result == B_OK)
		result = into->AddUInt32(kKeyAlertType, static_cast<uint32>(fType));
	return result;
}
