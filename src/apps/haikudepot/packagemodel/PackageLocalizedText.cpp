/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PackageLocalizedText.h"


PackageLocalizedText::PackageLocalizedText()
	:
	fTitle(),
	fSummary(),
	fDescription(),
	fHasChangelog(false),
	fChangelog()
{
}


PackageLocalizedText::PackageLocalizedText(const PackageLocalizedText& other)
	:
	fTitle(other.Title()),
	fSummary(other.Summary()),
	fDescription(other.Description()),
	fHasChangelog(other.HasChangelog()),
	fChangelog(other.Changelog())
{
}


const BString&
PackageLocalizedText::Title() const
{
	return fTitle;
}


void
PackageLocalizedText::SetTitle(const BString& value)
{
	fTitle = value;
}


const BString&
PackageLocalizedText::Summary() const
{
	return fSummary;
}


void
PackageLocalizedText::SetSummary(const BString& value)
{
	fSummary = value;
}


const BString&
PackageLocalizedText::Description() const
{
	return fDescription;
}


void
PackageLocalizedText::SetDescription(const BString& value)
{
	fDescription = value;
}


const bool
PackageLocalizedText::HasChangelog() const
{
	return fHasChangelog;
}


void
PackageLocalizedText::SetHasChangelog(bool value)
{
	fHasChangelog = value;

	if (!value)
		SetChangelog("");
}


void
PackageLocalizedText::SetChangelog(const BString& value)
{
	fChangelog = value;
	fHasChangelog = !value.IsEmpty();
}


const BString&
PackageLocalizedText::Changelog() const
{
	return fChangelog;
}


bool
PackageLocalizedText::operator==(const PackageLocalizedText& other) const
{
	return fTitle == other.Title() && fSummary == other.Summary()
		&& fDescription == other.Description() && fHasChangelog == other.HasChangelog()
		&& fChangelog == other.Changelog();
}


bool
PackageLocalizedText::operator!=(const PackageLocalizedText& other) const
{
	return !(*this == other);
}
