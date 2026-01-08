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


#endif // PACKAGE_ACTION_H
