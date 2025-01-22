/*
 * Copyright 2024-2025, Andrew Lindesay <apl@lindesay.co.nz>.
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


// #pragma mark - PackageLocalInfoBuilder


PackageLocalizedTextBuilder::PackageLocalizedTextBuilder()
	:
	fTitle(),
	fSummary(),
	fDescription(),
	fHasChangelog(false),
	fChangelog()
{
}


PackageLocalizedTextBuilder::PackageLocalizedTextBuilder(const PackageLocalizedTextRef& value)
	:
	fTitle(),
	fSummary(),
	fDescription(),
	fHasChangelog(false),
	fChangelog()
{
	fSource = value;
}


PackageLocalizedTextBuilder::~PackageLocalizedTextBuilder()
{
}


void
PackageLocalizedTextBuilder::_InitFromSource()
{
	if (fSource.IsSet()) {
		_Init(fSource);
		fSource.Unset();
	}
}


void
PackageLocalizedTextBuilder::_Init(const PackageLocalizedText* value)
{
	fTitle = value->Title();
	fSummary = value->Summary();
	fDescription = value->Description();
	fHasChangelog = value->HasChangelog();
	fChangelog = value->Changelog();
}


PackageLocalizedTextRef
PackageLocalizedTextBuilder::BuildRef()
{
	if (fSource.IsSet())
		return fSource;

	PackageLocalizedText* localizedText = new PackageLocalizedText();
	localizedText->SetTitle(fTitle);
	localizedText->SetSummary(fSummary);
	localizedText->SetDescription(fDescription);
	localizedText->SetHasChangelog(fHasChangelog);
	localizedText->SetChangelog(fChangelog);

	return PackageLocalizedTextRef(localizedText, true);
}


PackageLocalizedTextBuilder&
PackageLocalizedTextBuilder::WithTitle(const BString& value)
{
	if (!fSource.IsSet() || fSource->Title() != value) {
		_InitFromSource();
		fTitle = value;
	}
	return *this;
}


PackageLocalizedTextBuilder&
PackageLocalizedTextBuilder::WithSummary(const BString& value)
{
	if (!fSource.IsSet() || fSource->Summary() != value) {
		_InitFromSource();
		fSummary = value;
	}
	return *this;
}


PackageLocalizedTextBuilder&
PackageLocalizedTextBuilder::WithDescription(const BString& value)
{
	if (!fSource.IsSet() || fSource->Description() != value) {
		_InitFromSource();
		fDescription = value;
	}
	return *this;
}


PackageLocalizedTextBuilder&
PackageLocalizedTextBuilder::WithHasChangelog(bool value)
{
	if (!fSource.IsSet() || fSource->HasChangelog() != value) {
		_InitFromSource();
		fHasChangelog = value;
	}
	return *this;
}


PackageLocalizedTextBuilder&
PackageLocalizedTextBuilder::WithChangelog(const BString& value)
{
	if (!fSource.IsSet() || fSource->Changelog() != value) {
		_InitFromSource();
		fChangelog = value;
	}
	return *this;
}
