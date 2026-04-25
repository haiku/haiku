/*
 * Copyright 2021-2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_ACTION_H
#define PACKAGE_ACTION_H

#include <Message.h>
#include <Referenceable.h>
#include <String.h>

#include "DeskbarLink.h"
#include "ScreenshotCoordinate.h"


class PackageAction : public BReferenceable, public BArchivable
{
public:
								PackageAction(const BString& title, const BString& packageName);
								PackageAction(const BMessage* from);
	virtual						~PackageAction();

	const	BString&			Title() const;
	const	BString&			PackageName() const;

	const	BMessage			Message() const;
	virtual	const uint32		MessageWhat() const = 0;

	virtual	status_t			Archive(BMessage* into, bool deep = true) const;

protected:
			BString				fTitle;
			BString				fPackageName;
};


typedef BReference<PackageAction> PackageActionRef;


class UninstallPackageAction : public PackageAction
{
public:
								UninstallPackageAction(const BString& packageName,
									const BString& packageTitle);
								UninstallPackageAction(const BMessage* from);
	virtual						~UninstallPackageAction();

	virtual	const uint32		MessageWhat() const;

};


class InstallPackageAction : public PackageAction
{
public:
								InstallPackageAction(const BString& packageName,
                                	const BString& packageTitle);
								InstallPackageAction(const BMessage* from);
	virtual						~InstallPackageAction();

	virtual	const uint32		MessageWhat() const;
};


class OpenPackageAction : public PackageAction
{
public:
								OpenPackageAction(const BString& packageName,
									const DeskbarLink& link);
								OpenPackageAction(const BMessage* from);
								~OpenPackageAction();

	virtual	const uint32		MessageWhat() const;

	const	DeskbarLink			Link() const;

	virtual	status_t			Archive(BMessage* into, bool deep = true) const;

private:
			DeskbarLink			fLink;
};


class CacheScreenshotPackageAction : public PackageAction {
public:
								CacheScreenshotPackageAction(const BString& packageName,
									const ScreenshotCoordinate& screenshotCoordinate);
								CacheScreenshotPackageAction(const BMessage* from);
								~CacheScreenshotPackageAction();

	virtual	const uint32		MessageWhat() const;

			ScreenshotCoordinate
								Coordinate() const;

	virtual	status_t			Archive(BMessage* into, bool deep = true) const;

private:
			ScreenshotCoordinate
								fScreenshotCoordinate;
};


class PopulateChangelogPackageAction : public PackageAction {
public:
								PopulateChangelogPackageAction(const BString& packageName);
								PopulateChangelogPackageAction(const BMessage* from);
	virtual						~PopulateChangelogPackageAction();

	virtual	const uint32		MessageWhat() const;
};


class PopulateUserRatingsPackageAction : public PackageAction {
public:
								PopulateUserRatingsPackageAction(const BString& packageName);
								PopulateUserRatingsPackageAction(const BMessage* from);
	virtual						~PopulateUserRatingsPackageAction();

	virtual	const uint32		MessageWhat() const;
};


#endif // PACKAGE_ACTION_H
