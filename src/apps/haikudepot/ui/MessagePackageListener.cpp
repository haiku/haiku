/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "MessagePackageListener.h"

#include <Messenger.h>
#include <View.h>

#include "PackageInfo.h"


// #pragma mark - MessagePackageListener


MessagePackageListener::MessagePackageListener(BHandler* target)
	:
	fTarget(target),
	fChangesMask(0xffffffff)
{
}


MessagePackageListener::~MessagePackageListener()
{
}


void
MessagePackageListener::PackageChanged(const PackageInfoEvent& event)
{
	if ((event.Changes() & fChangesMask) == 0)
		return;

	BMessenger messenger(fTarget);
	if (!messenger.IsValid())
		return;

	BMessage message(MSG_UPDATE_PACKAGE);
	message.AddString("name", event.Package()->Name());
	message.AddUInt32("changes", event.Changes());

	messenger.SendMessage(&message);
}


void
MessagePackageListener::SetChangesMask(uint32 mask)
{
	fChangesMask = mask;
}


// #pragma mark - OnePackageMessagePackageListener


OnePackageMessagePackageListener::OnePackageMessagePackageListener(BHandler* target)
	:
	MessagePackageListener(target)
{
}


OnePackageMessagePackageListener::~OnePackageMessagePackageListener()
{
}


void
OnePackageMessagePackageListener::SetPackage(const PackageInfoRef& package)
{
	if (fPackage == package)
		return;

	PackageInfoListenerRef listener(this);

	if (fPackage.Get() != NULL)
		fPackage->RemoveListener(listener);

	fPackage = package;

	if (fPackage.Get() != NULL)
		fPackage->AddListener(listener);
}


const PackageInfoRef&
OnePackageMessagePackageListener::Package() const
{
	return fPackage;
}
