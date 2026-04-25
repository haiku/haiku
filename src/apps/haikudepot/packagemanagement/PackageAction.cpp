/*
 * Copyright 2021-2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "PackageAction.h"

#include <Catalog.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "PackageUtils.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageAction"


static const char* const kKeyTitle = "title";
static const char* const kKeyDeskbarLink = "deskbar_link";
static const char* const kKeyScreenshotCoordinates = "screenshot_coordinates";


// #pragma mark - PackageAction


/*!	An abstract superclass of the various sorts of package actions which can be
	undertaken on a package.
*/
PackageAction::PackageAction(const BString& title, const BString& packageName)
	:
	fTitle(title),
	fPackageName(packageName)
{
}


PackageAction::PackageAction(const BMessage* from)
{
	if (from->FindString(kKeyTitle, &fTitle) != B_OK)
		HDFATAL("expected key [%s] in message", kKeyTitle);
	if (from->FindString(shared_message_keys::kKeyPackageName, &fPackageName) != B_OK)
		HDFATAL("expected key [%s] in message", shared_message_keys::kKeyPackageName);
}


PackageAction::~PackageAction()
{
}


const BString&
PackageAction::Title() const
{
	return fTitle;
}


const BString&
PackageAction::PackageName() const
{
	return fPackageName;
}


/*!	Returns a `BMessage` which can be used in BView action to cause the action
	to get executed by some UI logic.
*/
const BMessage
PackageAction::Message() const
{
	BMessage message(MessageWhat());
	if (Archive(&message) != B_OK)
		HDFATAL("unable to archive the action message.");
	return message;
}


status_t
PackageAction::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	if (result == B_OK && into == NULL)
		result = B_ERROR;
	if (result == B_OK)
		result = into->AddString(kKeyTitle, fTitle);
	if (result == B_OK)
		result = into->AddString(shared_message_keys::kKeyPackageName, fPackageName);
	return result;
}


// #pragma mark - UninstallPackageAction


UninstallPackageAction::UninstallPackageAction(const BString& packageName,
	const BString& packageTitle)
	:
	PackageAction("Uninstall", packageName)
{
	fTitle = B_TRANSLATE("Uninstall %PackageTitle%");
	fTitle.ReplaceAll("%PackageTitle%", packageTitle);
}


UninstallPackageAction::UninstallPackageAction(const BMessage* from)
	:
	PackageAction(from)
{
}


UninstallPackageAction::~UninstallPackageAction()
{
}


const uint32
UninstallPackageAction::MessageWhat() const
{
	return MSG_PKG_UNINSTALL;
}


// #pragma mark - InstallPackageAction


InstallPackageAction::InstallPackageAction(const BString& packageName,
	const BString& packageTitle)
	:
	PackageAction("Install", packageName)
{
	fTitle = B_TRANSLATE("Install %PackageTitle%");
	fTitle.ReplaceAll("%PackageTitle%", packageTitle);
}


InstallPackageAction::InstallPackageAction(const BMessage* from)
	:
	PackageAction(from)
{
}


InstallPackageAction::~InstallPackageAction()
{
}


const uint32
InstallPackageAction::MessageWhat() const
{
	return MSG_PKG_INSTALL;
}


// #pragma mark - OpenPackageAction


OpenPackageAction::OpenPackageAction(const BString& packageName, const DeskbarLink& deskbarLink)
	:
	PackageAction("Open", packageName),
	fLink(deskbarLink)
{
	fTitle = B_TRANSLATE("Open %DeskbarLink%");
	fTitle.ReplaceAll("%DeskbarLink%", deskbarLink.Title());
}


OpenPackageAction::OpenPackageAction(const BMessage* from)
	:
	PackageAction(from)
{
	BMessage deskbarLinkMessage;
	if (from->FindMessage(kKeyDeskbarLink, &deskbarLinkMessage) == B_OK)
		fLink = DeskbarLink(&deskbarLinkMessage);
	else
		HDFATAL("missing key [%s]", kKeyDeskbarLink);
}


OpenPackageAction::~OpenPackageAction()
{
}


const uint32
OpenPackageAction::MessageWhat() const
{
	return MSG_PKG_OPEN;
}


const DeskbarLink
OpenPackageAction::Link() const
{
	return fLink;
}


status_t
OpenPackageAction::Archive(BMessage* into, bool deep) const
{
	status_t result = PackageAction::Archive(into, deep);

	if (result == B_OK) {
		BMessage deskbarLinkMessage;
		result = fLink.Archive(&deskbarLinkMessage);
		if (result == B_OK)
			result = into->AddMessage(kKeyDeskbarLink, &deskbarLinkMessage);
	}

	return result;
}


// #pragma mark - CacheScreenshotPackageAction


CacheScreenshotPackageAction::CacheScreenshotPackageAction(const BString& packageName,
	const ScreenshotCoordinate& screenshotCoordinate)
	:
	PackageAction("Screenshot", packageName),
	fScreenshotCoordinate(screenshotCoordinate)
{
}


CacheScreenshotPackageAction::CacheScreenshotPackageAction(const BMessage* from)
	:
	PackageAction(from)
{
	BMessage screenshotCoordinateMessage;
	if (from->FindMessage(kKeyScreenshotCoordinates, &screenshotCoordinateMessage) == B_OK)
		fScreenshotCoordinate = ScreenshotCoordinate(&screenshotCoordinateMessage);
	else
		HDFATAL("missing key [%s]", kKeyScreenshotCoordinates);
}


CacheScreenshotPackageAction::~CacheScreenshotPackageAction()
{
}


const uint32
CacheScreenshotPackageAction::MessageWhat() const
{
	return MSG_PKG_CACHE_SCREENSHOT;
}


ScreenshotCoordinate
CacheScreenshotPackageAction::Coordinate() const
{
	return fScreenshotCoordinate;
}


status_t
CacheScreenshotPackageAction::Archive(BMessage* into, bool deep) const
{
	status_t result = PackageAction::Archive(into, deep);

	if (result == B_OK) {
		BMessage screenshotCoordinateMessage;
		result = fScreenshotCoordinate.Archive(&screenshotCoordinateMessage);
		if (result == B_OK)
			result = into->AddMessage(kKeyScreenshotCoordinates, &screenshotCoordinateMessage);
	}

	return result;
}


// #pragma mark - PopulateChangelogPackageAction


PopulateChangelogPackageAction::PopulateChangelogPackageAction(const BString& packageName)
	:
	PackageAction("Populate Changelog", packageName)
{
}


PopulateChangelogPackageAction::PopulateChangelogPackageAction(const BMessage* from)
	:
	PackageAction(from)
{
}


PopulateChangelogPackageAction::~PopulateChangelogPackageAction()
{
}


const uint32
PopulateChangelogPackageAction::MessageWhat() const
{
	return MSG_PKG_POPULATE_CHANGELOG;
}


// #pragma mark - PopulateUserRatingsPackageAction


PopulateUserRatingsPackageAction::PopulateUserRatingsPackageAction(const BString& packageName)
	:
	PackageAction("Populate User Ratings", packageName)
{
}


PopulateUserRatingsPackageAction::PopulateUserRatingsPackageAction(const BMessage* from)
	:
	PackageAction(from)
{
}


PopulateUserRatingsPackageAction::~PopulateUserRatingsPackageAction()
{
}


const uint32
PopulateUserRatingsPackageAction::MessageWhat() const
{
	return MSG_PKG_POPULATE_USER_RATINGS;
}
