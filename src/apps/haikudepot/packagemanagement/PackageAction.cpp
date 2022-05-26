/*
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PackageAction.h"


PackageAction::PackageAction(const BString& title, const BMessage& message)
	:
	fTitle(title),
	fMessage(message)
{
}


PackageAction::~PackageAction()
{
}


const BString&
PackageAction::Title() const
{
	return fTitle;
}


const BMessage&
PackageAction::Message() const
{
	return fMessage;
}
