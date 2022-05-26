/*
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_ACTION_H
#define PACKAGE_ACTION_H


#include <Message.h>
#include <Referenceable.h>
#include <String.h>


class PackageAction : public BReferenceable {
public:
								PackageAction(const BString& title,
									const BMessage& message);
	virtual						~PackageAction();

	const	BString&			Title() const;
	const	BMessage&			Message() const;

private:
			BString				fTitle;
			BMessage			fMessage;
};


typedef BReference<PackageAction> PackageActionRef;


#endif // PACKAGE_ACTION_H
