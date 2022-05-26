/*
 * Copyright 2013-2021, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Rene Gollent <rene@gollent.com>
 *		Julian Harnath <julian.harnath@rwth-aachen.de>
 *		Andrew Lindesay <apl@lindesay.co.nz>
 *
 * Note that this file has been re-factored from `PackageManager.cpp` and
 * authors have been carried across in 2021.
 */


#include "DeskbarLink.h"

#include "Logger.h"


#define kPathKey	"path"
#define kLinkKey	"link"


DeskbarLink::DeskbarLink()
{
}


DeskbarLink::DeskbarLink(const BString& path, const BString& link)
	:
	fPath(path),
	fLink(link)
{
}


DeskbarLink::DeskbarLink(const DeskbarLink& other)
	:
	fPath(other.fPath),
	fLink(other.fLink)
{
}


DeskbarLink::DeskbarLink(BMessage* from)
{
	if (from->FindString(kPathKey, &fPath) != B_OK) {
		HDERROR("expected key [%s] in the message data when creating a "
			"captcha", kPathKey);
	}

	if (from->FindString(kLinkKey, &fLink) != B_OK) {
		HDERROR("expected key [%s] in the message data when creating a "
			"captcha", kLinkKey);
	}
}



DeskbarLink::~DeskbarLink()
{
}


const BString
DeskbarLink::Path() const
{
	return fPath;
}


const BString
DeskbarLink::Link() const
{
	return fLink;
}


status_t
DeskbarLink::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	if (result == B_OK && into == NULL)
		result = B_ERROR;
	if (result == B_OK)
		result = into->AddString(kPathKey, fPath);
	if (result == B_OK)
		result = into->AddString(kLinkKey, fLink);
	return result;
}


DeskbarLink&
DeskbarLink::operator=(const DeskbarLink& other)
{
	if (this == &other)
		return *this;
	fPath = other.Path();
	fLink = other.Link();
	return *this;
}


bool
DeskbarLink::operator==(const DeskbarLink& other)
{
	return fPath == other.fPath && fLink == other.fLink;
}


bool
DeskbarLink::operator!=(const DeskbarLink& other)
{
	return !(*this == other);
}
