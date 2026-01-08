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
